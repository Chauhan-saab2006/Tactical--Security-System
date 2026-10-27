import React from 'react';
import { Activity, Thermometer, ShieldAlert, BarChart3, MapPin, Info } from 'lucide-react';
import '../Layout/AlertsPanel.css'; // Reusing some panel styling

const EarthquakePrediction = ({ onClose }) => {
    const predictions = [
        { id: 1, region: 'Himalayan Belt', likelihood: 'High', magnitude: '5.2 - 6.1', timeFrame: 'Next 48 Hours', riskColor: 'var(--color-brand-red)' },
        { id: 2, region: 'Indo-Gangetic Plain', likelihood: 'Moderate', magnitude: '3.4 - 4.2', timeFrame: 'Next 7 Days', riskColor: 'var(--color-brand-orange)' },
        { id: 3, region: 'Deccan Plateau', likelihood: 'Low', magnitude: '2.1 - 2.8', timeFrame: 'Stable', riskColor: 'var(--color-brand-green)' }
    ];

    return (
        <aside className="alerts-panel">
            <div className="panel-header">
                <div className="header-title-row">
                    <Activity className="header-icon" />
                    <h2>Earthquake Prediction</h2>
                </div>
                <button className="close-panel-btn" onClick={onClose}>
                    <ShieldAlert size={18} />
                </button>
            </div>

            <div className="panel-stats-grid" style={{ padding: '0 20px', marginBottom: '20px' }}>
                <div className="stat-mini-card">
                    <span className="stat-label">Seismic Index</span>
                    <span className="stat-value text-red">Critical</span>
                </div>
                <div className="stat-mini-card">
                    <span className="stat-label">Active Gaps</span>
                    <span className="stat-value">12</span>
                </div>
            </div>

            <div className="alerts-scroll-area">
                <div className="section-divider">ACTIVE MONITORING</div>

                {predictions.map(pred => (
                    <div key={pred.id} className="alert-card" style={{ borderLeftColor: pred.riskColor }}>
                        <div className="alert-card-header">
                            <div className="user-info">
                                <MapPin size={16} color={pred.riskColor} />
                                <div className="user-text">
                                    <span className="user-name">{pred.region}</span>
                                    <span className="device-id">Risk Level: {pred.likelihood}</span>
                                </div>
                            </div>
                            <div className="alert-time">{pred.timeFrame}</div>
                        </div>

                        <div className="alert-vitals">
                            <div className="vital-item">
                                <Activity size={14} />
                                <span>{pred.magnitude} Mw</span>
                            </div>
                            <div className="vital-item">
                                <BarChart3 size={14} />
                                <span>{pred.likelihood} Confidence</span>
                            </div>
                        </div>

                        <div className="alert-actions">
                            <button className="dispatch-btn" style={{ background: pred.riskColor }}>
                                View Hotspots
                            </button>
                        </div>
                    </div>
                ))}

                <div className="info-box" style={{ margin: '20px', padding: '15px', background: 'rgba(0,255,255,0.05)', borderRadius: '12px', border: '1px solid rgba(0,255,255,0.1)' }}>
                    <div style={{ display: 'flex', gap: '10px', alignItems: 'flex-start' }}>
                        <Info size={16} color="var(--color-brand-cyan)" />
                        <p style={{ fontSize: '11px', color: 'var(--text-secondary)', margin: 0 }}>
                            Predictions are based on real-time sensory data from groundwater levels and radon gas emission sensors located at key nodes.
                        </p>
                    </div>
                </div>
            </div>
        </aside>
    );
};

export default EarthquakePrediction;
