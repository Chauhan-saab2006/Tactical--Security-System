import React from 'react';
import IconSidebar from './IconSidebar';
import './MainLayout.css';

const MainLayout = ({ children, activeTab, onTabChange }) => {
    return (
        <div className="main-layout">
            <IconSidebar activeTab={activeTab} onTabChange={onTabChange} />
            <div className="content-area">
                {children}
            </div>
        </div>
    );
};

export default MainLayout;
