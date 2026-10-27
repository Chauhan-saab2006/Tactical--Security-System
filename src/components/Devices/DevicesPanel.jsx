import React, { useState, useEffect } from 'react';
import { Smartphone, Battery, Signal, Info, X, RefreshCw, Activity } from 'lucide-react';
import { fetchDevicesStatus } from '../../services/api';
import './DevicesPanel.css';
import '../../styles/transitions.css';

const DevicesPanel = ({ onClose, deviceTelemetry = {} }) => {
    const [devices, setDevices] = useState([]);
    const [loading, setLoading] = useState(true);
    const [error, setError] = useState(null);

    const loadDevices = async () => {
        try {
            setLoading(true);
            const data = await fetchDevicesStatus();
            // Transform backend data to UI format
            const formattedDevices = data.map(device => {
                const lastSeenDate = device.last_seen ? new Date(device.last_seen) : null;
                const now = new Date();
                const diffMinutes = lastSeenDate ? (now - lastSeenDate) / 1000 / 60 : Infinity;

                // Use backend is_online as primary, fallback to local calculation
                let connectivityStatus = 'Offline';
                if (device.is_online || diffMinutes < 5) connectivityStatus = 'Online';
                else if (diffMinutes < 60) connectivityStatus = 'Idle';

                return {
                    id: device.device_id,
                    name: device.device_id === 'midway_panel' ? 'Midway Gateway' : 'SOS Tracker',
                    user: 'Registered Node',
                    battery: '90%', // Default as backend doesn't store battery yet
                    status: device.sos_active ? 'SOS ACTIVE' : connectivityStatus,
                    signal: connectivityStatus === 'Online' ? 'Strong' : 'None',
                    lastSeen: lastSeenDate ? lastSeenDate.toLocaleTimeString() : 'Never',
                    pulse: device.last_pulse || 0,
                    lat: device.last_latitude,
                    lng: device.last_longitude
                };
            });
            setDevices(formattedDevices);
            setError(null);
        } catch (err) {
            console.error("Error loading devices:", err);
            setError("Failed to sync with hardware network.");
        } finally {
            setLoading(false);
        }
    };

    useEffect(() => {
        loadDevices();
        const interval = setInterval(loadDevices, 30000); // Refresh every 30s
        return () => clearInterval(interval);
    }, []);

    return (
        <div className="devices-panel animate-slide-in">
            <header className="panel-header">
                <div className="header-flex">
                    <h1>Hardware Connectivity</h1>
                    <div className="header-actions">
                        <button className="refresh-btn" onClick={loadDevices} title="Refresh Status">
                            <RefreshCw size={18} className={loading ? 'spin' : ''} />
                        </button>
                        <button className="panel-close-btn" onClick={onClose} title="Close Panel">
                            <X size={20} />
                        </button>
                    </div>
                </div>
                <p>Real-time status of nodes and gateways in the Antigravity network.</p>
            </header>

            {error && (
                <div className="error-banner">
                    <Info size={16} /> {error}
                </div>
            )}

            <div className="devices-grid">
                {loading && devices.length === 0 ? (
                    <div className="loading-state">Syncing hardware...</div>
                ) : devices.length === 0 ? (
                    <div className="empty-devices">
                        <Signal size={40} />
                        <h3>No Devices Found</h3>
                        <p>No hardware nodes have checked into the system yet.</p>
                    </div>
                ) : (
                    devices.map(device => (
                        <div key={device.id} className={`device-card ${device.status === 'SOS ACTIVE' ? 'alert' : ''}`}>
                            <div className="device-header">
                                <div className="device-icon-box">
                                    <Smartphone size={20} />
                                </div>
                                <div className="device-meta">
                                    <h3>{device.id}</h3>
                                    <span>{device.name}</span>
                                </div>
                                <div className="device-status">
                                    <span className={`status-pill ${device.status.toLowerCase().replace(' ', '-')}`}>
                                        {device.status}
                                    </span>
                                </div>
                            </div>

                            <div className="device-body">
                                <div className="body-row">
                                    <span className="label">Last Heartbeat</span>
                                    <span className="value">{deviceTelemetry[device.id]?.timestamp ? new Date(deviceTelemetry[device.id].timestamp).toLocaleTimeString() : device.lastSeen}</span>
                                </div>
                                <div className="body-stats">
                                    <div className="stat pulse-stat">
                                        <Activity size={14} className="pulse-icon" />
                                        <span className="pulse-value">{deviceTelemetry[device.id]?.pulse || device.pulse || '--'} BPM</span>
                                    </div>
                                    <div className="stat">
                                        <Battery size={14} />
                                        <span>{device.battery}</span>
                                    </div>
                                    <div className="stat">
                                        <Signal size={14} />
                                        <span>{device.signal}</span>
                                    </div>
                                </div>
                            </div>
                        </div>
                    ))
                )}
            </div>
        </div>
    );
};

export default DevicesPanel;
