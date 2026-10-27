import React from 'react';
import { Shield, LayoutGrid, Bell, Map as MapIcon, Settings, HelpCircle, LogOut, Smartphone, Users, Activity } from 'lucide-react';
import './IconSidebar.css';

const IconSidebar = ({ activeTab, onTabChange }) => {
    return (
        <aside className="icon-sidebar">
            <div className="sidebar-top">
                <div className="logo-container">
                    <div className="logo-icon">
                        <Shield size={28} color="var(--color-brand-green)" fill="var(--color-brand-green)" />
                    </div>
                </div>

                <nav className="sidebar-nav">
                    <button
                        className={`nav-item ${activeTab === 'dashboard' ? 'active' : ''}`}
                        onClick={() => onTabChange('dashboard')}
                        title="Dashboard"
                    >
                        <LayoutGrid size={24} />
                    </button>
                    <button
                        className={`nav-item has-badge ${activeTab === 'alerts' ? 'active' : ''}`}
                        onClick={() => onTabChange('alerts')}
                        title="Alerts"
                    >
                        <Bell size={24} />
                        <span className="badge-dot"></span>
                    </button>
                    <button
                        className={`nav-item ${activeTab === 'map' ? 'active' : ''}`}
                        onClick={() => onTabChange('map')}
                        title="Map View"
                    >
                        <MapIcon size={24} />
                    </button>
                    <button
                        className={`nav-item ${activeTab === 'devices' ? 'active' : ''}`}
                        onClick={() => onTabChange('devices')}
                        title="Devices"
                    >
                        <Smartphone size={24} />
                    </button>
                    <button
                        className={`nav-item ${activeTab === 'users' ? 'active' : ''}`}
                        onClick={() => onTabChange('users')}
                        title="Users Management"
                    >
                        <Users size={24} />
                    </button>
                    <button
                        className={`nav-item ${activeTab === 'earthquake' ? 'active' : ''}`}
                        onClick={() => onTabChange('earthquake')}
                        title="Earthquake Prediction"
                    >
                        <Activity size={24} />
                    </button>
                    <button
                        className={`nav-item ${activeTab === 'settings' ? 'active' : ''}`}
                        onClick={() => onTabChange('settings')}
                        title="Settings"
                    >
                        <Settings size={24} />
                    </button>
                </nav>
            </div>

            <div className="sidebar-bottom">
                <button className="nav-item" title="Help">
                    <HelpCircle size={24} />
                </button>
                <button className="nav-item" title="Logout">
                    <LogOut size={24} />
                </button>
            </div>
        </aside>
    );
};

export default IconSidebar;
