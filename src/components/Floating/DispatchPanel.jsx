import React, { useState } from 'react';
import { Check, Activity, Navigation } from 'lucide-react';
import './DispatchPanel.css';

const DispatchPanel = ({ alert, onSendMessage }) => {
    const [isDispatched, setIsDispatched] = useState(false);

    if (!alert) return null;

    const handleDispatch = () => {
        setIsDispatched(true);
    };

    return (
        <div className="dispatch-panel">
            <div className="panel-header-row">
                <div className="header-left">
                    <div className="user-name-group">
                        <h2>User: {alert.user}</h2>
                         <span className={`sos-pill ${alert.type} device-${alert.device}`}>
                            {alert.type === 'sos' ? 'ACTIVE SOS' : alert.status}
                        </span>
                    </div>

                    <div className="vitals-header-group">
                        <div className="vital-pill pulse">
                            <Activity size={14} className="icon" />
                            <div className="pill-content">
                                <span className="label">HEART BEAT</span>
                                <span className="value">{alert.details?.pulse || 'N/A'}</span>
                            </div>
                        </div>
                        <div className="vital-pill speed">
                            <Navigation size={14} className="icon" />
                            <div className="pill-content">
                                <span className="label">SPEED</span>
                                <span className="value">{alert.details?.speed || 'N/A'}</span>
                            </div>
                        </div>
                    </div>
                </div>
                <div className="header-actions">
                    <button className="action-btn secondary" onClick={onSendMessage}>
                        SEND MESSAGE
                    </button>
                    <button
                        className={`dispatch-btn ${isDispatched ? 'dispatched' : ''}`}
                        onClick={handleDispatch}
                        disabled={isDispatched}
                    >
                        {isDispatched ? (
                            <>
                                <Check size={16} /> Response Dispatched
                            </>
                        ) : (
                            <>
                                <Check size={16} /> DISPATCH RESPONSE
                            </>
                        )}
                    </button>
                </div>
            </div>

            <div className="info-grid">
                <div className="info-item">
                    <label>DEVICE ID</label>
                    <div className="value-group">
                        <span className="main-value">{alert.device}</span>
                        <span className="sub-value">Suraksha Bio-Tracker</span>
                    </div>
                </div>

                <div className="info-item">
                    <label>CURRENT LOCATION</label>
                    <div className="value-group">
                        <span className="main-value">{alert.destination}</span>
                        <span className="sub-value">Lat: {alert.position[0].toFixed(4)}, Lng: {alert.position[1].toFixed(4)}</span>
                    </div>
                </div>

                <div className="info-item">
                    <label>CURRENT STATUS</label>
                    <div className="value-group">
                        <span className="main-value">{alert.status}</span>
                        <span className="sub-value">Sensor: {alert.details?.sensor || 'N/A'}</span>
                    </div>
                </div>

                <div className="info-item">
                    <label>DISTANCE</label>
                    <div className="value-group">
                        <span className="main-value">0.85 km</span>
                        <span className="sub-value">To Command Hub</span>
                    </div>
                </div>

                <div className="info-item">
                    <label>BATTERY</label>
                    <div className="value-group">
                        <span className={`main-value ${parseInt(alert.details?.battery) > 20 ? 'highlight-green' : 'highlight-red'}`}>
                            {alert.details?.battery || 'N/A'}
                        </span>
                        <span className="sub-value">Syncing Live</span>
                    </div>
                </div>
            </div>
        </div>
    );
};

export default DispatchPanel;
