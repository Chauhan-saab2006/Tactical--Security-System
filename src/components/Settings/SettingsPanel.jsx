import React from 'react';
import { User, Bell, Shield, Moon, Globe, X } from 'lucide-react';
import './SettingsPanel.css';
import '../../styles/transitions.css';

const SettingsPanel = ({ darkMode, onThemeToggle, onClose }) => {
    return (
        <div className="settings-panel animate-slide-in">
            <header className="panel-header">
                <div className="header-flex">
                    <h1>System Settings</h1>
                    <button className="panel-close-btn" onClick={onClose} title="Close Panel">
                        <X size={20} />
                    </button>
                </div>
                <p>Configure platform preferences and security parameters.</p>
            </header>

            <div className="settings-sections">
                <div className="settings-group">
                    <h3>Account Profile</h3>
                    <div className="setting-item">
                        <div className="setting-info">
                            <User size={18} />
                            <div>
                                <p className="setting-title">Admin Profile</p>
                                <p className="setting-desc">Manage your personal information and credentials.</p>
                            </div>
                        </div>
                        <button className="settings-action">Edit</button>
                    </div>
                </div>


                <div className="settings-group">
                    <h3>Appearance</h3>
                    <div className="setting-item">
                        <div className="setting-info">
                            <Moon size={18} />
                            <div>
                                <p className="setting-title">Dark Mode</p>
                                <p className="setting-desc">Switch between light and dark visual themes.</p>
                            </div>
                        </div>
                        <div
                            className={`toggle ${darkMode ? 'active' : ''}`}
                            onClick={onThemeToggle}
                        ></div>
                    </div>
                </div>
            </div>
        </div>
    );
};

export default SettingsPanel;
