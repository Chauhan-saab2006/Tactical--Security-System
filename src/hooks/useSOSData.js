import { useState, useEffect, useRef, useCallback } from 'react';
import { fetchActiveSOS, fetchDevicesStatus } from '../services/api';
import { connectSOSWebSocket } from '../services/websocket';

export const useSOSData = () => {
  const [alerts, setAlerts] = useState([]);
  const [checkpoints, setCheckpoints] = useState([]);
  const [deviceTelemetry, setDeviceTelemetry] = useState({});
  const [selectedAlertId, setSelectedAlertId] = useState(null);
  const wsConnected = useRef(false);
  const pollInterval = useRef(null);
  const lastPollData = useRef(null);

  // Polling fallback: fetches device status + active SOS via REST API
  const pollBackend = useCallback(async () => {
    try {
      // Poll devices status
      const devices = await fetchDevicesStatus();
      if (devices && Array.isArray(devices)) {
        const newTelemetry = {};
        devices.forEach(d => {
          newTelemetry[d.device_id] = {
            pulse: d.last_pulse,
            latitude: d.last_latitude,
            longitude: d.last_longitude,
            isOnline: d.is_online,
            lastSeen: d.last_seen,
            sosActive: d.sos_active,
            timestamp: d.last_seen
          };
        });
        setDeviceTelemetry(prev => ({ ...prev, ...newTelemetry }));
      }

      // Poll active SOS events
      const activeSOS = await fetchActiveSOS();
      const currentHash = JSON.stringify(activeSOS);
      if (currentHash !== lastPollData.current) {
        lastPollData.current = currentHash;
        if (activeSOS && activeSOS.length > 0) {
          const formattedAlerts = activeSOS.map(event => ({
            id: event.id,
            type: 'sos',
            user: `User ${event.device_id}`,
            avatar: '/avatars/male_2.png',
            destination: 'Live Location',
            device: event.device_id,
            status: 'SOS ACTIVE',
            position: [event.location_data?.[0]?.latitude || 30.268423, event.location_data?.[0]?.longitude || 77.993792],
            details: {
              pulse: event.pulse_data?.[0]?.bpm ? `${event.pulse_data[0].bpm} BPM` : 'N/A',
              heartbeat: 'Just now',
              speed: '1.2 km/h',
              battery: '90%',
              sensor: 'GPS Tracking'
            },
            team: {
              status: 'DISPATCHED',
              eta: '8 mins'
            },
            assigned: 'Amit Verma'
          }));

          setAlerts(prev => {
            // Merge: update existing alerts by device, add new ones
            const merged = [...prev];
            formattedAlerts.forEach(newAlert => {
              const idx = merged.findIndex(a => a.device === newAlert.device || a.id === newAlert.id);
              if (idx !== -1) {
                merged[idx] = { ...merged[idx], ...newAlert };
              } else {
                merged.unshift(newAlert);
              }
            });
            return merged;
          });
        }
      }
    } catch (error) {
      console.warn("Poll fallback failed:", error.message);
    }
  }, []);

  // Start/stop polling based on WebSocket connectivity
  const startPolling = useCallback(() => {
    if (pollInterval.current) return;
    console.log("WebSocket down — starting REST polling fallback (every 5s)");
    pollBackend(); // immediate first poll
    pollInterval.current = setInterval(pollBackend, 5000);
  }, [pollBackend]);

  const stopPolling = useCallback(() => {
    if (pollInterval.current) {
      console.log("WebSocket connected — stopping REST polling");
      clearInterval(pollInterval.current);
      pollInterval.current = null;
    }
  }, []);

  // Load Initial SOS Data
  useEffect(() => {
    const loadInitialData = async () => {
      try {
        const activeSOS = await fetchActiveSOS();
        if (activeSOS.length > 0) {
          const formattedAlerts = activeSOS.map(event => ({
            id: event.id,
            type: 'sos',
            user: `User ${event.device_id}`,
            avatar: '/avatars/male_2.png',
            destination: 'Live Location',
            device: event.device_id,
            status: 'SOS ACTIVE',
            position: [event.location_data?.[0]?.latitude || 30.268423, event.location_data?.[0]?.longitude || 77.993792],
            details: {
              pulse: event.pulse_data?.[0]?.bpm ? `${event.pulse_data[0].bpm} BPM` : 'N/A',
              heartbeat: 'Just now',
              speed: '1.2 km/h',
              battery: '90%',
              sensor: 'GPS Tracking'
            },
            team: {
              status: 'DISPATCHED',
              eta: '8 mins'
            },
            assigned: 'Amit Verma'
          }));
          setAlerts(prev => [...formattedAlerts, ...prev]);
        }
      } catch (error) {
        console.error("Failed to load active SOS:", error);
      }
    };

    loadInitialData();

    // Setup WebSocket with polling fallback
    let reconnectAttempt = 0;
    let reconnectTimer = null;
    let ws = null;

    const setupWebSocket = () => {
      ws = connectSOSWebSocket((data) => {
        console.log("WebSocket Message Received:", data);

        if (data.type === 'sos_alert' || data.type === 'telemetry_update' || data.type === 'pulse_alert') {
          if (data.device_id) {
            setDeviceTelemetry(prev => ({
              ...prev,
              [data.device_id]: {
                pulse: data.pulse,
                latitude: data.latitude,
                longitude: data.longitude,
                timestamp: data.timestamp || new Date().toISOString()
              }
            }));
          }

          setAlerts(prev => {
            const existingIndex = prev.findIndex(a => (data.event_id && a.id === data.event_id) || (!data.event_id && a.device === data.device_id));

            if (existingIndex !== -1) {
              const updated = [...prev];
              const item = {
                ...updated[existingIndex],
                position: data.latitude ? [data.latitude, data.longitude] : updated[existingIndex].position,
                status: 'SOS ACTIVE',
                details: {
                  ...updated[existingIndex].details,
                  pulse: data.pulse !== undefined ? `${data.pulse} BPM` : updated[existingIndex].details.pulse,
                  heartbeat: 'Just now',
                  battery: data.battery !== undefined ? `${data.battery}%` : updated[existingIndex].details.battery
                }
              };
              updated.splice(existingIndex, 1);
              return [item, ...updated];
            } else if (data.sos || data.type === 'sos_alert') {
              const newId = data.event_id || Date.now();
              setSelectedAlertId(newId);
              return [{
                id: newId,
                type: 'sos',
                user: data.device_id === 'midway_panel' ? 'Midway Panel' : `User ${data.device_id}`,
                avatar: data.device_id === 'midway_panel' ? '/avatars/male_1.png' : '/avatars/male_2.png',
                device: data.device_id,
                status: 'SOS ACTIVE',
                position: data.latitude ? [data.latitude, data.longitude] : [30.268423, 77.993792],
                details: {
                  pulse: data.pulse !== undefined ? `${data.pulse} BPM` : 'N/A',
                  heartbeat: 'Just now',
                  speed: data.device_id === 'midway_panel' ? null : '0.0 km/h',
                  battery: data.battery !== undefined ? `${data.battery}%` : '92%',
                  sensor: data.device_id === 'midway_panel' ? 'Control Gateway' : 'GPS Tracking'
                },
                team: { status: 'DISPATCHED', eta: '12 mins' },
                assigned: 'Quick Response Team'
              }, ...prev];
            }
            return prev;
          });
        } else if (data.type === 'checkpoint_reached') {
          const newCheckpoint = {
            id: Date.now(),
            user: data.user_id || 'Unknown User',
            location: data.device_id,
            time: new Date().toLocaleTimeString()
          };

          setCheckpoints(prev => [newCheckpoint, ...prev]);
          setTimeout(() => {
            setCheckpoints(prev => prev.filter(cp => cp.id !== newCheckpoint.id));
          }, 5000);
        }
      });

      // Override onopen/onclose/onerror for polling fallback management
      const originalOnOpen = ws.onopen;
      ws.onopen = (e) => {
        console.log("WebSocket connected successfully");
        wsConnected.current = true;
        reconnectAttempt = 0;
        stopPolling();
        if (originalOnOpen) originalOnOpen(e);
      };

      const originalOnClose = ws.onclose;
      ws.onclose = (e) => {
        console.log("WebSocket disconnected — activating polling fallback");
        wsConnected.current = false;
        startPolling();
        // Don't use the built-in reconnect from websocket.js; manage here with backoff
      };

      const originalOnError = ws.onerror;
      ws.onerror = (e) => {
        console.warn("WebSocket error — polling fallback active");
        wsConnected.current = false;
        startPolling();
      };

      return ws;
    };

    ws = setupWebSocket();

    // Start polling immediately as a safety net (stop once WS connects)
    startPolling();

    return () => {
      stopPolling();
      if (reconnectTimer) clearTimeout(reconnectTimer);
      if (ws) ws.close();
    };
  }, [startPolling, stopPolling]);

  return {
    alerts,
    setAlerts,
    checkpoints,
    deviceTelemetry,
    selectedAlertId,
    setSelectedAlertId
  };
};
