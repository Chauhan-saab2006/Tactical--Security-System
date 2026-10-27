import React, { useEffect, useState } from 'react';
import { Activity, Shield, Cpu, Zap, X } from 'lucide-react';
import './DashboardPanel.css';
import '../../styles/transitions.css';

const CountUpValue = ({ end, suffix = "" }) => {
    const [count, setCount] = useState(0);

    useEffect(() => {
        let start = 0;
        const duration = 1000;
        const increment = end / (duration / 16);

        const timer = setInterval(() => {
            start += increment;
            if (start >= end) {
                setCount(end);
                clearInterval(timer);
            } else {
                setCount(Math.floor(start));
            }
        }, 16);

        return () => clearInterval(timer);
    }, [end]);

    return <span>{count}{suffix}</span>;
};

const DashboardPanel = ({ onClose, alerts = [], stats: propStats }) => {
    // Default stats if not provided via props
    const defaultStats = {
        activeSos: alerts.filter(a => a.type === 'sos').length,
        responders: 16,
        deviceHealth: 96,
        uptime: '99.98%'
    };

    const displayStats = propStats || defaultStats;

    return (
        <div className="dashboard-panel animate-slide-in">
            <header className="panel-header">
                <div className="header-flex">
                    <h1>Suraksha Command Dashboard</h1>
                    <div className="live-badge">
                        <span className="pulse-dot"></span>
                        LIVE SYNC
                    </div>
                    <button className="panel-close-btn" onClick={onClose} title="Close Dashboard">
                        <X size={20} />
                    </button>
                </div>
                <p>Real-time Indian safety monitoring and emergency response.</p>
            </header>

            <div className="stats-grid">
                <div className="stat-card">
                    <div className="stat-icon sos">
                        <Shield size={24} />
                    </div>
                    <div className="stat-info">
                        <span className="stat-label">Active SOS</span>
                        <div className="stat-value">
                            <CountUpValue end={displayStats.activeSos} />
                        </div>
                    </div>
                </div>

                <div className="stat-card">
                    <div className="stat-icon responder">
                        <Activity size={24} />
                    </div>
                    <div className="stat-info">
                        <span className="stat-label">Responders</span>
                        <div className="stat-value">
                            <CountUpValue end={displayStats.responders} />
                        </div>
                    </div>
                </div>

                <div className="stat-card">
                    <div className="stat-icon device">
                        <Cpu size={24} />
                    </div>
                    <div className="stat-info">
                        <span className="stat-label">Device Health</span>
                        <div className="stat-value">
                            <CountUpValue end={displayStats.deviceHealth} suffix="%" />
                        </div>
                    </div>
                </div>

                <div className="stat-card">
                    <div className="stat-icon uptime">
                        <Zap size={24} />
                    </div>
                    <div className="stat-info">
                        <span className="stat-label">System Uptime</span>
                        <div className="stat-value">{displayStats.uptime}</div>
                    </div>
                </div>
            </div>

            <section className="activity-feed">
                <h3>Live Feed</h3>
                <div className="feed-list">
                    {[1, 2, 3].map(i => (
                        <div key={i} className="feed-item">
                            <div className="feed-dot"></div>
                            <div className="feed-content">
                                <p><strong>Yantra #IND-{(100 + i)}</strong> connection established. Sensors active.</p>
                                <span className="feed-time">Just now</span>
                            </div>
                        </div>
                    ))}
                </div>
            </section>
        </div>
    );
};

export default DashboardPanel;
