import { useState, useEffect, useRef } from 'react';
import { ref, onValue, off } from 'firebase/database';
import { database } from '../services/firebase';

/**
 * Decodes the epoch-millisecond timestamp baked into a Firebase push key.
 * Firebase push() keys encode the server-side arrival time in the first 8
 * characters using a custom base64 alphabet.
 * This gives us the EXACT time the ESP's data reached Firebase — even when
 * the ESP firmware sends NO timestamp field at all.
 *
 * Returns 0 if the key is not a valid push key.
 */
const FIREBASE_PUSH_CHARS =
  '-0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz';

function decodePushKeyTimestamp(pushKey) {
  if (!pushKey || pushKey.length < 8) return 0;
  try {
    let timestamp = 0;
    for (let i = 0; i < 8; i++) {
      const charIndex = FIREBASE_PUSH_CHARS.indexOf(pushKey[i]);
      if (charIndex === -1) return 0; // not a Firebase push key
      timestamp = timestamp * 64 + charIndex;
    }
    return timestamp; // epoch-milliseconds
  } catch {
    return 0;
  }
}

/**
 * Formats a raw Firebase SOS record into the alert shape used by AlertsPanel.
 * Accepts any key structure under sos_alerts/{id}.
 *
 * Expected (flexible) Firebase fields:
 *   tower / tower_id / node / node_id / device_id / deviceId / device
 *   lat / latitude / location.lat
 *   lng / longitude / location.lng
 *   pulse / heart_rate / bpm
 *   battery
 *   timestamp / created_at   ← optional; push key used as fallback
 *   status
 *   avatar / avatarUrl
 *   assigned / assigned_to
 */
const formatFirebaseSOS = (id, data) => {
  // Tower / device name — shown as the main heading on the alert card
  const tower =
    data.tower || data.tower_id || data.node || data.node_id ||
    data.device_id || data.deviceId || data.device || `Tower ${id}`;

  const lat = parseFloat(data.lat ?? data.latitude ?? data.location?.lat ?? 30.2684);
  const lng = parseFloat(data.lng ?? data.longitude ?? data.location?.lng ?? 77.9938);

  const pulseRaw = data.pulse ?? data.heart_rate ?? data.bpm;
  const pulseStr = pulseRaw !== undefined ? `${pulseRaw} BPM` : 'N/A';

  const battery = data.battery !== undefined ? `${data.battery}%` : 'N/A';

  const deviceId = data.device_id || data.deviceId || data.device || id;

  const avatar = data.avatar || data.avatarUrl || '/avatars/male_2.png';

  // ─── Timestamp resolution (priority order) ───────────────────────────────
  // 1. Explicit field sent by ESP firmware      → data.timestamp
  // 2. Alternative explicit field               → data.created_at
  // 3. Firebase push key decode (AUTOMATIC)     → decodePushKeyTimestamp(id)
  //    The push key encodes the millisecond server-side arrival time, so this
  //    works with zero changes to the ESP firmware.
  // ─────────────────────────────────────────────────────────────────────────
  const tsRaw = data.timestamp ?? data.created_at ?? decodePushKeyTimestamp(id);

  // Normalise to epoch-ms (Firebase sometimes stores Unix seconds, 10 digits)
  const tsMs = tsRaw
    ? (typeof tsRaw === 'number' && tsRaw < 1e12 ? tsRaw * 1000 : Number(tsRaw))
    : 0;

  // Human-readable date + time string for the alert card
  let heartbeat = 'N/A';
  if (tsMs > 0) {
    const d = new Date(tsMs);
    heartbeat = isNaN(d.getTime())
      ? String(tsRaw)
      : d.toLocaleString('en-IN', {
          day:    '2-digit',
          month:  'short',
          year:   'numeric',
          hour:   '2-digit',
          minute: '2-digit',
          second: '2-digit',
          hour12: true,
        });
  }

  return {
    id,
    _source:  'firebase',
    _ts:       tsMs,              // epoch-ms — used for sorting (newest first)
    type:     'sos',
    user:      tower,             // main heading on the alert card
    avatar,
    destination: 'Live Location',
    device:    deviceId,          // small meta label on the card
    status:    data.status || 'SOS ACTIVE',
    position:  [lat, lng],
    details: {
      pulse:     pulseStr,
      heartbeat,                  // "15 Aug 2026, 12:24:05 PM" — from push key if no field
      speed:     data.speed ? `${data.speed} km/h` : 'N/A',
      battery,
      sensor:    'GPS Tracking',
    },
    team: {
      status: data.team_status || 'DISPATCHED',
      eta:    data.eta          || 'Calculating…',
    },
    assigned: data.assigned || data.assigned_to || 'Quick Response Team',
  };
};


/**
 * useFirebaseSOS
 *
 * Subscribes in real-time to the `sos_alerts` node in Firebase Realtime Database.
 * Returns:
 *   sosAlerts   – array of formatted alert objects (newest first)
 *   loading     – true while the first snapshot is pending
 *   error       – error message string, or null
 */
export const useFirebaseSOS = () => {
  const [sosAlerts, setSosAlerts] = useState([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);
  const listenerRef = useRef(null);

  useEffect(() => {
    if (!database) {
      setLoading(false);
      setError('Firebase not configured. Add VITE_FIREBASE_* variables to .env');
      return;
    }

    const sosRef = ref(database, 'LifeLink-SosAlert');
    listenerRef.current = sosRef;

    const unsubscribe = onValue(
      sosRef,
      (snapshot) => {
        setLoading(false);
        setError(null);

        const val = snapshot.val();
        if (!val) {
          setSosAlerts([]);
          return;
        }

        // val is an object: { [pushKey]: { ...fields } }
        const formatted = Object.entries(val).map(([id, data]) =>
          formatFirebaseSOS(id, data)
        );

        // Sort newest SOS first: compare epoch-ms, fall back to push key order (desc)
        formatted.sort((a, b) => {
          if (b._ts !== a._ts) return b._ts - a._ts;
          return b.id > a.id ? 1 : -1;
        });

        setSosAlerts(formatted);
      },
      (err) => {
        console.error('[useFirebaseSOS] Listener error:', err);
        setLoading(false);
        setError(err.message);
      }
    );

    return () => {
      off(sosRef);
    };
  }, []);

  return { sosAlerts, loading, error };
};
