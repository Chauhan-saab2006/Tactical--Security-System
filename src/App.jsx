import React, { useState, useEffect } from 'react'
import MainLayout from './components/Layout/MainLayout'
import AlertsPanel from './components/Layout/AlertsPanel'
import MapContainer from './components/Map/MapContainer'
import SearchHeader from './components/Floating/SearchHeader'
import DispatchPanel from './components/Floating/DispatchPanel'
import MessageModal from './components/Modals/MessageModal'
import DashboardPanel from './components/Dashboard/DashboardPanel'
import EarthquakePrediction from './components/Dashboard/EarthquakePrediction'
import DevicesPanel from './components/Devices/DevicesPanel'
import SettingsPanel from './components/Settings/SettingsPanel'
import UsersPanel from './components/Users/UsersPanel'
import './App.css'

import { useSOSData } from './hooks/useSOSData'
import { useFirebaseSOS } from './hooks/useFirebaseSOS'

function App() {
  const [activeTab, setActiveTab] = useState('alerts');
  const [darkMode, setDarkMode] = useState(true);
  const [zones, setZones] = useState([]);
  const [isMessageModalOpen, setIsMessageModalOpen] = useState(false);
  const [messageRecipient, setMessageRecipient] = useState(null);

  const {
    alerts,
    setAlerts,
    checkpoints,
    deviceTelemetry,
    selectedAlertId,
    setSelectedAlertId
  } = useSOSData();

  // Firebase Realtime Database — SOS alerts
  const { sosAlerts: firebaseSOS, loading: firebaseLoading, error: firebaseError } = useFirebaseSOS();

  // History data for side panel display metrics
  const [history, setHistory] = useState([
    { id: 101, user: 'Sanjay Dutt', avatar: '/avatars/male_1.png', location: 'Dhela Range', time: '10:15 AM', responseTime: '12 min', status: 'RESOLVED' },
    { id: 102, user: 'Priya Rai', avatar: '/avatars/female_1.png', location: 'Garjiya Gate', time: '09:30 AM', responseTime: '18 min', status: 'RESOLVED' },
  ]);

  const addZone = (zone) => {
    setZones(prev => [...prev, { ...zone, id: Date.now() }]);
  };

  const handleSelectAlert = (id) => {
    setSelectedAlertId(id);
    setActiveTab('alerts');
  };

  const handleSendMessage = (user) => {
    setMessageRecipient(user);
    setIsMessageModalOpen(true);
  };

  useEffect(() => {
    document.documentElement.setAttribute('data-theme', darkMode ? 'dark' : 'light');
  }, [darkMode]);

  // Global Zoom Blocker to prevent whole website from zooming
  useEffect(() => {
    const preventZoom = (e) => {
      if (e.ctrlKey || (e.type === 'wheel' && Math.abs(e.deltaY) < 10)) {
        if (!e.target.closest('.map-container')) {
          e.preventDefault();
        }
      }
    };
    window.addEventListener('wheel', preventZoom, { passive: false });
    return () => window.removeEventListener('wheel', preventZoom);
  }, []);

  const [mapType, setMapType] = useState('Satellite');

  const selectedAlert = alerts.find(a => a.id === selectedAlertId) || alerts.find(a => a.type === 'sos');

  const renderSidePanel = () => {
    const commonProps = { onClose: () => setActiveTab('map') };
    switch (activeTab) {
      case 'dashboard':
        return <DashboardPanel alerts={alerts} {...commonProps} />;
      case 'alerts':
        return <AlertsPanel alerts={alerts} history={history} selectedId={selectedAlertId} onSelectAlert={handleSelectAlert} firebaseSOS={firebaseSOS} firebaseLoading={firebaseLoading} firebaseError={firebaseError} {...commonProps} />;
      case 'earthquake':
        return <EarthquakePrediction {...commonProps} />;
      case 'devices':
        return <DevicesPanel deviceTelemetry={deviceTelemetry} {...commonProps} />;
      case 'settings':
        return <SettingsPanel darkMode={darkMode} onThemeToggle={() => setDarkMode(!darkMode)} {...commonProps} />;
      case 'users':
        return <UsersPanel {...commonProps} />;
      default:
        return null;
    }
  };

  return (
    <MainLayout activeTab={activeTab} onTabChange={(tab) => {
      if (tab === activeTab && tab !== 'map') {
        setActiveTab('map');
      } else if (['dashboard', 'alerts', 'map', 'settings', 'devices', 'users', 'earthquake'].includes(tab)) {
        setActiveTab(tab);
      }
    }}>
      <div className={`main-content-split ${activeTab === 'map' ? 'view-map' : 'view-panel'}`}>
        {activeTab !== 'map' && (
          <div className="side-panel-container">
            {renderSidePanel()}
          </div>
        )}
        <div className={`map-wrapper ${activeTab === 'map' ? 'full-width' : 'partial-width'}`}>
          <MapContainer
            alerts={alerts}
            firebaseSOS={firebaseSOS}
            mapType={mapType}
            onSelectAlert={handleSelectAlert}
            onSendMessage={(user) => handleSendMessage(user)}
            selectedId={selectedAlertId}
            zones={zones}
            onAddZone={addZone}
          />
          <SearchHeader mapType={mapType} onMapTypeChange={setMapType} deviceTelemetry={deviceTelemetry} />
          {selectedAlert && (
            <DispatchPanel
              alert={selectedAlert}
              onSendMessage={() => handleSendMessage(selectedAlert.user)}
            />
          )}
        </div>
      </div>

      <MessageModal
        isOpen={isMessageModalOpen}
        onClose={() => setIsMessageModalOpen(false)}
        recipient={messageRecipient}
      />

      {/* Checkpoint Notifications Overlay */}
      <div className="checkpoint-overlay">
        {checkpoints.map(cp => (
          <div key={cp.id} className="checkpoint-toast">
            <div className="checkpoint-toast-icon">📍</div>
            <div className="checkpoint-toast-content">
              <strong>Checkpoint Reached</strong>
              <span>{cp.user} checked in at {cp.location}</span>
              <small>{cp.time}</small>
            </div>
          </div>
        ))}
      </div>
    </MainLayout>
  )
}

export default App
