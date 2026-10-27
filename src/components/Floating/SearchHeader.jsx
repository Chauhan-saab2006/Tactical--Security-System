import React, { useState } from 'react';
import { Search, CornerUpRight } from 'lucide-react';
import './SearchHeader.css';

const SearchHeader = ({ mapType, onMapTypeChange, deviceTelemetry = {} }) => {
    // Check if any telemetry was received from the gateway in the last 90 seconds
    const gwPing = deviceTelemetry['midway_panel']?.timestamp;
    const isLive = gwPing && (new Date() - new Date(gwPing)) < 90000;

    return (
        <div className="search-header-container">
            <div className={`hardware-status-badge ${isLive ? 'live' : 'offline'}`}>
                <div className={`status-dot ${isLive ? 'pulse' : ''}`}></div>
                <span>{isLive ? 'Hardware Live' : 'Hardware Offline'}</span>
            </div>
            <div className="search-bar">
                <Search size={20} className="search-icon" />
                <input
                    type="text"
                    placeholder="Search address or user..."
                    className="search-input"
                />
                <div className="search-action">
                    <CornerUpRight size={18} />
                </div>
            </div>


        </div>
    );
};

export default SearchHeader;
