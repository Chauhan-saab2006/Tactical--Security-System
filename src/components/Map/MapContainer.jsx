import React, { useState, useEffect, useCallback, useRef } from 'react';
import {
    MapContainer as LeafletMap,
    TileLayer,
    Marker,
    Popup,
    Circle,
    Polyline,
    useMap
} from 'react-leaflet';
import L from 'leaflet';
import 'leaflet/dist/leaflet.css';
import {
    Plus,
    Minus,
    ShieldAlert,
    Forward,
    User,
    Activity,
    MapPin,
    HandMetal,
    Target,
    X,
    Check,
    Send,
    Map,
    Clock,
    Navigation,
    CornerUpRight,
    CornerUpLeft,
    ArrowUp,
    ChevronRight,
    Search
} from 'lucide-react';

import './MapContainer.css';

// Fix Leaflet Default Icon issue
import icon from 'leaflet/dist/images/marker-icon.png';
import iconShadow from 'leaflet/dist/images/marker-shadow.png';

let DefaultIcon = L.icon({
    iconUrl: icon,
    shadowUrl: iconShadow,
    iconSize: [25, 41],
    iconAnchor: [12, 41]
});
L.Marker.prototype.options.icon = DefaultIcon;

// Custom Marker Icons using Leaflet.divIcon
const createCustomIcon = (className) => L.divIcon({
    className: 'custom-leaflet-icon',
    html: `<div class="${className}"></div>`,
    iconSize: [30, 30],
    iconAnchor: [15, 15]
});

const createInfaIcon = (IconComponent, color) => L.divIcon({
    className: 'custom-infa-icon',
    html: `<div style="color: ${color}; display: flex; align-items: center; justify-content: center; background: rgba(0,0,0,0.5); border-radius: 50%; width: 30px; height: 30px; border: 1px solid ${color};">
        <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">${IconComponent}</svg>
    </div>`,
    iconSize: [30, 30],
    iconAnchor: [15, 15]
});

// Firebase Node Icon — shield marker with glowing ring (matches reference image)
const createNodeIcon = (status = 'active') => {
    const isActive = status === 'SOS ACTIVE' || status === 'active';
    const color = isActive ? '#4A90D9' : '#6B7280';
    const glowColor = isActive ? 'rgba(74, 144, 217, 0.35)' : 'rgba(107, 114, 128, 0.25)';
    return L.divIcon({
        className: 'firebase-node-icon',
        html: `<div class="node-marker-wrap ${isActive ? 'active' : 'inactive'}">
            <div class="node-glow-ring"></div>
            <div class="node-shield-core">
                <svg width="20" height="20" viewBox="0 0 24 24" fill="${color}" stroke="#FFD700" stroke-width="1.5">
                    <path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"/>
                </svg>
            </div>
        </div>`,
        iconSize: [44, 44],
        iconAnchor: [22, 22]
    });
};

const MapEvents = ({ onMouseDown, onMouseMove, onMouseUp, isDrawingMode }) => {
    const map = useMap();

    useEffect(() => {
        if (!map) return;

        const handleMouseDown = (e) => {
            if (isDrawingMode) {
                L.DomEvent.stopPropagation(e);
                onMouseDown(e.latlng);
            }
        };

        const handleMouseMove = (e) => {
            if (isDrawingMode) {
                onMouseMove(e.latlng);
            }
        };

        const handleMouseUp = (e) => {
            if (isDrawingMode) {
                onMouseUp(e.latlng);
            }
        };

        map.on('mousedown', handleMouseDown);
        map.on('mousemove', handleMouseMove);
        map.on('mouseup', handleMouseUp);

        if (isDrawingMode) {
            map.dragging.disable();
        } else {
            map.dragging.enable();
        }

        return () => {
            map.off('mousedown', handleMouseDown);
            map.off('mousemove', handleMouseMove);
            map.off('mouseup', handleMouseUp);
            map.dragging.enable();
        };
    }, [map, isDrawingMode, onMouseDown, onMouseMove, onMouseUp]);

    return null;
};

const MapFollower = ({ selectedPos }) => {
    const map = useMap();
    useEffect(() => {
        if (selectedPos && map) {
            map.flyTo(selectedPos, 16, {
                duration: 1.5,
                easeLinearity: 0.25
            });
        }
    }, [selectedPos, map]);
    return null;
};

const MapContainer = ({
    alerts = [],
    firebaseSOS = [],
    mapType = 'Map',
    onSelectAlert,
    onSendMessage,
    selectedId,
    zones = [],
    onAddZone
}) => {
    const CGCCenter = [28.462274, 77.490619]; 
    const [isDrawingMode, setIsDrawingMode] = useState(false);
    const [selectedAlertId, setSelectedAlertId] = useState(null);
    const [activeRoute, setActiveRoute] = useState(null);
    const [routeInfo, setRouteInfo] = useState(null);
    const [navigationSteps, setNavigationSteps] = useState([]);
    const [isNavPanelOpen, setIsNavPanelOpen] = useState(true);

    // Fetch Road Route and Steps using OSRM
    const getRoadDirections = async (start, end) => {
        try {
            const response = await fetch(
                `https://router.project-osrm.org/route/v1/driving/${start[1]},${start[0]};${end[1]},${end[0]}?overview=full&geometries=geojson&steps=true`
            );
            const data = await response.json();

            if (data.routes && data.routes.length > 0) {
                const route = data.routes[0];
                const coordinates = route.geometry.coordinates.map(coord => [coord[1], coord[0]]);
                setActiveRoute(coordinates);

                // Travel time and distance
                const durationMinutes = Math.round(route.duration / 60);
                const distanceKm = (route.distance / 1000).toFixed(1);
                setRouteInfo({
                    duration: durationMinutes,
                    distance: distanceKm
                });

                // Extract turn-by-turn steps
                const steps = route.legs[0].steps.map(step => {
                    const bearing = step.maneuver.bearing_after;
                    let cardinal = 'N';
                    if (bearing >= 337.5 || bearing < 22.5) cardinal = 'N';
                    else if (bearing >= 22.5 && bearing < 67.5) cardinal = 'NE';
                    else if (bearing >= 67.5 && bearing < 112.5) cardinal = 'E';
                    else if (bearing >= 112.5 && bearing < 157.5) cardinal = 'SE';
                    else if (bearing >= 157.5 && bearing < 202.5) cardinal = 'S';
                    else if (bearing >= 202.5 && bearing < 247.5) cardinal = 'SW';
                    else if (bearing >= 247.5 && bearing < 292.5) cardinal = 'W';
                    else if (bearing >= 292.5 && bearing < 337.5) cardinal = 'NW';

                    return {
                        instruction: step.maneuver.instruction,
                        distance: Math.round(step.distance),
                        type: step.maneuver.type,
                        modifier: step.maneuver.modifier,
                        cardinal: cardinal
                    };
                });
                setNavigationSteps(steps);
            }
        } catch (error) {
            console.error("Routing Error:", error);
            setActiveRoute([start, end]);
            setRouteInfo(null);
            setNavigationSteps([]);
        }
    };

    // Auto-fetch route when alert is selected or directions clicked
    useEffect(() => {
        const idToFocus = selectedId || selectedAlertId;
        const selected = alerts.find(a => a.id === idToFocus);

        if (selected && !idToFocus?.toString().startsWith('team-')) {
            getRoadDirections(ROUTING_ORIGIN, selected.position);
        } else if (!idToFocus && alerts.length > 0) {
            // Default to first SOS alert path if nothing selected
            const firstSOS = alerts.find(a => a.type === 'sos');
            if (firstSOS) {
                getRoadDirections(ROUTING_ORIGIN, firstSOS.position);
            }
        } else {
            setActiveRoute(null);
            setRouteInfo(null);
            setNavigationSteps([]);
        }
    }, [selectedId, selectedAlertId, alerts]);
    const [isConfiguring, setIsConfiguring] = useState(false);
    const [zoneName, setZoneName] = useState('');
    const [zoneType, setZoneType] = useState('safe');
    const [newZoneCenter, setNewZoneCenter] = useState(null);
    const [newZoneRadius, setNewZoneRadius] = useState(0);
    const [isDragging, setIsDragging] = useState(false);
    const [dragStart, setDragStart] = useState(null);

    const zoneColors = {
        safe: '#2E7D32',
        restricted: '#1A237E',
        danger: '#D32F2F'
    };

    const INFRASTRUCTURE_NODES = [
        { id: 'node-main', position: [28.462274, 77.490619], name: 'Main Node — Dhoolkot, Selaqui', radius: 500 },
        { id: 'node-2',    position: [28.460414, 77.492289], name: 'Node 2 — 250m SE of University', radius: 500 },
    ];

    // Routing origin defaults to the Main Node
    const ROUTING_ORIGIN = INFRASTRUCTURE_NODES[0].position;

    const handleMouseDown = useCallback((latlng) => {
        setDragStart(latlng);
        setNewZoneCenter([latlng.lat, latlng.lng]);
        setNewZoneRadius(0);
        setIsDragging(true);
    }, []);

    const handleMouseMove = useCallback((latlng) => {
        if (isDragging && dragStart) {
            const center = L.latLng(dragStart);
            const current = L.latLng(latlng);
            const radius = center.distanceTo(current);
            setNewZoneRadius(Math.round(radius));
        }
    }, [isDragging, dragStart]);

    const handleMouseUp = useCallback((latlng) => {
        if (isDragging) {
            setIsDragging(false);
            setDragStart(null);
            if (newZoneRadius > 10) { // Only open if it's more than a tiny click
                setIsConfiguring(true);
            } else {
                setNewZoneCenter(null);
                setNewZoneRadius(0);
            }
        }
    }, [isDragging, newZoneRadius]);

    const handleSaveZone = () => {
        if (!zoneName || !newZoneCenter) return;
        onAddZone({
            name: zoneName,
            type: zoneType,
            center: newZoneCenter,
            radius: newZoneRadius
        });
        setIsConfiguring(false);
        setIsDrawingMode(false);
        setNewZoneCenter(null);
        setNewZoneRadius(0);
        setZoneName('');
    };

    const selectedAlert = alerts.find(a => a.id === selectedAlertId);

    const shieldSvg = '<path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z" />';

    return (
        <div className={`map-container ${isDrawingMode ? 'drawing-cursor' : ''}`}>
            {isDrawingMode && !isConfiguring && (
                <div className="drawing-hint">
                    Click and drag on the map to draw a new zone
                </div>
            )}

            <LeafletMap
                center={CGCCenter}
                zoom={12}
                zoomControl={false}
                scrollWheelZoom={true}
                className="leaflet-map-element"
                style={{ width: '100%', height: '100%' }}
            >
                <TileLayer
                    url={mapType === 'Satellite'
                        ? 'https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}'
                        : 'https://{s}.basemaps.cartocdn.com/dark_all/{z}/{y}/{x}.png'}
                    attribution='&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors'
                />

                <MapEvents
                    onMouseDown={handleMouseDown}
                    onMouseMove={handleMouseMove}
                    onMouseUp={handleMouseUp}
                    isDrawingMode={isDrawingMode && !isConfiguring}
                />

                <MapFollower selectedPos={selectedAlert?.position} />



                {/* Infrastructure Nodes */}
                {INFRASTRUCTURE_NODES.map(node => (
                    <React.Fragment key={node.id}>
                        <Circle
                            center={node.position}
                            radius={node.radius}
                            pathOptions={{
                                color: node.id === 'node-main' ? 'black' : 'white',
                                // color: node.id === 'node-main' ? '#2E7D32' : '#1A237E',
                                weight: 2,
                                opacity: 1,
                                fillColor: node.id === 'node-main' ? 'red' : 'blue',
                                fillOpacity: 0.1
                            }}
                        />
                        <Marker
                            position={node.position}
                            icon={createInfaIcon(shieldSvg, node.id === 'node-main' ? 'red' : 'blue')}
                        >
                            <Popup>
                                <div className="map-popup-content">
                                    <h4>{node.name}</h4>
                                    <p><strong>Lat:</strong> {node.position[0].toFixed(5)}</p>
                                    <p><strong>Lng:</strong> {node.position[1].toFixed(5)}</p>
                                    <p><strong>Radius:</strong> {node.radius}m</p>
                                </div>
                            </Popup>
                        </Marker>
                    </React.Fragment>
                ))}

                {/* Firebase SOS Nodes — only the latest (most recent) node */}
                {firebaseSOS.slice(0, 1).map(node => (
                    <React.Fragment key={`fb-node-${node.id}`}>
                        <Marker
                            position={node.position}
                            icon={createNodeIcon(node.status)}
                            eventHandlers={{
                                click: () => {
                                    setSelectedAlertId(node.id);
                                    onSelectAlert && onSelectAlert(node.id);
                                },
                            }}
                        >
                            <Popup>
                                <div className="map-popup-content">
                                    <h4 style={{ color: '#4A90D9' }}>
                                        <span style={{ marginRight: 6 }}>🛡️</span>
                                        {node.user}
                                    </h4>
                                    <p><strong>Status:</strong> {node.status}</p>
                                    <p><strong>Device:</strong> {node.device}</p>
                                    <p><strong>Lat:</strong> {node.position[0].toFixed(5)}</p>
                                    <p><strong>Lng:</strong> {node.position[1].toFixed(5)}</p>
                                    {node.details && (
                                        <>
                                            <p><strong>Pulse:</strong> {node.details.pulse}</p>
                                            <p><strong>Battery:</strong> {node.details.battery}</p>
                                            <p><strong>Last Seen:</strong> {node.details.heartbeat}</p>
                                        </>
                                    )}
                                </div>
                            </Popup>
                        </Marker>
                    </React.Fragment>
                ))}

                {/* Zones */}
                {zones.map(zone => (
                    <Circle
                        key={zone.id}
                        center={zone.center}
                        radius={zone.radius}
                        pathOptions={{
                            color: zoneColors[zone.type],
                            weight: 2,
                            opacity: 0.8,
                            fillColor: zoneColors[zone.type],
                            fillOpacity: 0.2
                        }}
                    />
                ))}

                {/* New Zone Preview */}
                {(isConfiguring || isDragging) && newZoneCenter && (
                    <Circle
                        center={newZoneCenter}
                        radius={newZoneRadius}
                        pathOptions={{
                            color: zoneColors[zoneType],
                            weight: 3,
                            opacity: 0.9,
                            fillColor: zoneColors[zoneType],
                            fillOpacity: 0.3,
                            dashArray: '5, 5'
                        }}
                    />
                )}

                {/* SOS Alerts */}
                {alerts.map(alert => (
                    <React.Fragment key={alert.id}>
                        <Marker
                            position={alert.position}
                            icon={L.divIcon({
                                className: 'sos-marker-leaflet',
                                html: `
                                    <div class="sos-marker-container device-${alert.device} user-${alert.user ? alert.user.toLowerCase().replace(/[^a-z0-9]/g, '-') : ''}">
                                        <div class="radar-ping"></div>
                                        <div class="radar-ping-slow"></div>
                                        <div class="sos-dot-inner"></div>
                                    </div>
                                `,
                                iconSize: [40, 40],
                                iconAnchor: [20, 20]
                            })}
                            eventHandlers={{
                                click: () => {
                                    setSelectedAlertId(alert.id);
                                    onSelectAlert && onSelectAlert(alert.id);
                                },
                            }}
                        />

                        {/* Team Marker & Route */}
                        {alert.team && alert.team.position && (
                            <>
                                <Marker
                                    position={alert.team.position}
                                    icon={createInfaIcon(shieldSvg, '#2E7D32')}
                                    eventHandlers={{
                                        click: () => setSelectedAlertId(`team-${alert.id}`),
                                    }}
                                />
                                {alert.team.route && (
                                    <Polyline
                                        positions={alert.team.route}
                                        pathOptions={{
                                            color: '#2E7D32',
                                            weight: 3,
                                            opacity: 0.6,
                                            dashArray: '10, 10'
                                        }}
                                    />
                                )}
                            </>
                        )}

                        {/* Road Direction Line (Dashed Gold) - Only if selected */}
                        {selectedAlertId === alert.id && activeRoute && (
                            <Polyline
                                positions={activeRoute}
                                pathOptions={{
                                    color: '#FFD700',
                                    weight: 5,
                                    opacity: 0.9,
                                    dashArray: '1, 10',
                                    lineJoin: 'round',
                                    lineCap: 'round'
                                }}
                            />
                        )}
                    </React.Fragment>
                ))}

                {/* Popups */}
                {selectedAlert && !selectedAlertId?.toString()?.startsWith('team-') && (
                    <Popup
                        position={selectedAlert.position}
                        onClose={() => setSelectedAlertId(null)}
                    >
                        <div className="map-popup-content">
                             <h4 style={{ 
                                 color: selectedAlert.type === 'sos' 
                                     ? (['A', 'a', 'node-a', 'node_a', 'tower-a', 'tower_a'].includes(selectedAlert.device)
                                         ? '#FFFFFF' 
                                         : (['B', 'b', 'node-b', 'node_b', 'tower-b', 'tower_b'].includes(selectedAlert.device)
                                             ? 'var(--color-brand-red)'
                                             : 'var(--color-brand-orange)'))
                                     : 'inherit' 
                             }}>
                                {selectedAlert.user}
                            </h4>
                            <p><strong>Status:</strong> {selectedAlert.status}</p>
                            <p><strong>Location:</strong> {selectedAlert.destination}</p>

                            {routeInfo && (
                                <div className="eta-info-row">
                                    <div className="eta-item">
                                        <Clock size={12} />
                                        <span>{routeInfo.duration} mins</span>
                                    </div>
                                    <div className="eta-item">
                                        <Map size={12} />
                                        <span>{routeInfo.distance} km</span>
                                    </div>
                                </div>
                            )}

                            <div className="popup-actions-row">
                                <button
                                    className="action-btn-link"
                                    onClick={() => onSelectAlert && onSelectAlert(selectedAlert.id)}
                                >
                                    View Details
                                </button>
                                <button
                                    className="action-btn-link message"
                                    onClick={(e) => {
                                        e.stopPropagation();
                                        onSendMessage && onSendMessage(selectedAlert.user);
                                    }}
                                >
                                    <Send size={12} /> Send Message
                                </button>
                            </div>
                        </div>
                    </Popup>
                )}

                {selectedAlertId?.toString()?.startsWith('team-') && alerts.find(a => `team-${a.id}` === selectedAlertId) && (() => {
                    const teamAlert = alerts.find(a => `team-${a.id}` === selectedAlertId);
                    return (
                        <Popup
                            position={teamAlert.team?.position || teamAlert.position}
                            onClose={() => setSelectedAlertId(null)}
                        >
                            <div className="map-popup-content">
                                <h4>{teamAlert.team.id}</h4>
                                <p><strong>Status:</strong> {teamAlert.team.status}</p>
                                <p><strong>Headed to:</strong> {teamAlert.user}</p>
                                <p><strong>ETA:</strong> {teamAlert.team.eta}</p>
                                <div className="popup-actions-row">
                                    <button className="action-btn-link forward">
                                        <Forward size={12} /> Msg Team
                                    </button>
                                </div>
                            </div>
                        </Popup>
                    );
                })()}

            </LeafletMap>

            {/* Navigation Panel Overlay */}
            {activeRoute && navigationSteps.length > 0 && (
                <div
                    className={`navigation-panel ${isNavPanelOpen ? 'open' : 'closed'}`}
                    onClick={() => !isNavPanelOpen && setIsNavPanelOpen(true)}
                    style={{ cursor: !isNavPanelOpen ? 'pointer' : 'default' }}
                >
                    <div className="nav-header">
                        <div className="nav-title">
                            <Navigation size={18} className="nav-icon-main" />
                            <h3>Rescue Directions</h3>
                        </div>
                        <div className="nav-actions">
                            <button
                                className="nav-toggle-btn"
                                onClick={(e) => {
                                    e.stopPropagation();
                                    setIsNavPanelOpen(!isNavPanelOpen);
                                }}
                                title={isNavPanelOpen ? "Minimize" : "Expand"}
                            >
                                {isNavPanelOpen ? <Minus size={18} /> : <Navigation size={18} />}
                            </button>
                            <button
                                className="nav-close-btn"
                                onClick={(e) => {
                                    e.stopPropagation();
                                    setActiveRoute(null);
                                    setNavigationSteps([]);
                                    setSelectedAlertId(null);
                                    onSelectAlert && onSelectAlert(null);
                                }}
                            >
                                <X size={18} />
                            </button>
                        </div>
                    </div>

                    {isNavPanelOpen && (
                        <div className="nav-content">
                            <div className="nav-summary">
                                <div className="summary-item">
                                    <Clock size={16} />
                                    <span className="summary-val">{routeInfo?.duration} min</span>
                                </div>
                                <div className="summary-item">
                                    <Map size={16} />
                                    <span className="summary-val">{routeInfo?.distance} km</span>
                                </div>
                            </div>

                            <div className="steps-container">
                                {navigationSteps.map((step, idx) => (
                                    <div key={idx} className="step-item">
                                        <div className="step-icon-box">
                                            {step.modifier?.includes('left') ? <CornerUpLeft size={16} /> :
                                                step.modifier?.includes('right') ? <CornerUpRight size={16} /> :
                                                    <ArrowUp size={16} />}
                                        </div>
                                        <div className="step-details">
                                            <div className="step-header-row">
                                                <p className="step-instruction">{step.instruction}</p>
                                                <span className="cardinal-badge">{step.cardinal}</span>
                                            </div>
                                            <span className="step-dist">{step.distance} m</span>
                                        </div>
                                    </div>
                                ))}
                                <div className="step-item final">
                                    <div className="step-icon-box arrival">
                                        <Target size={16} />
                                    </div>
                                    <div className="step-details">
                                        <p className="step-instruction">Arrive at Emergency Location</p>
                                    </div>
                                </div>
                            </div>
                        </div>
                    )}
                </div>
            )}

            <div className="zone-tool-container" style={{ zIndex: 1000 }}>
                <button
                    className={`zone-tool-btn ${isDrawingMode ? 'active' : ''}`}
                    onClick={() => setIsDrawingMode(!isDrawingMode)}
                    title="Mark Circle Zone"
                >
                    <Target size={24} />
                    <span className="tool-label">{isDrawingMode ? 'MARKING...' : 'MARK ZONE'}</span>
                </button>
            </div>

            {/* Zone Config Modal */}
            {isConfiguring && (
                <div className="zone-modal-overlay">
                    <div className="zone-modal">
                        <div className="modal-header">
                            <h3>Configure New Zone</h3>
                            <button onClick={() => setIsConfiguring(false)}><X size={20} /></button>
                        </div>
                        <div className="modal-body">
                            <div className="input-group">
                                <label>Zone Name</label>
                                <input
                                    type="text"
                                    placeholder="e.g. Danger Zone 1"
                                    value={zoneName}
                                    onChange={(e) => setZoneName(e.target.value)}
                                    autoFocus
                                />
                            </div>
                            <div className="input-group">
                                <label>Zone Type</label>
                                <div className="type-selector">
                                    {Object.keys(zoneColors).map(type => (
                                        <button
                                            key={type}
                                            className={`type-btn ${type} ${zoneType === type ? 'selected' : ''}`}
                                            onClick={() => setZoneType(type)}
                                        >
                                            {type.charAt(0).toUpperCase() + type.slice(1)}
                                        </button>
                                    ))}
                                </div>
                            </div>
                            <div className="input-group">
                                <div className="label-with-value">
                                    <label>Zone Radius</label>
                                    <span className="radius-value">{(newZoneRadius / 1000).toFixed(1)} km</span>
                                </div>
                                <div className="radius-controls">
                                    <input
                                        type="range"
                                        min="100"
                                        max="10000"
                                        step="100"
                                        value={newZoneRadius}
                                        onChange={(e) => setNewZoneRadius(parseInt(e.target.value))}
                                        className="radius-slider"
                                    />
                                    <input
                                        type="number"
                                        value={newZoneRadius}
                                        onChange={(e) => setNewZoneRadius(parseInt(e.target.value) || 0)}
                                        className="radius-input"
                                    />
                                    <span className="unit-label">meters</span>
                                </div>
                            </div>
                        </div>
                        <div className="modal-footer">
                            <button className="cancel-btn" onClick={() => setIsConfiguring(false)}>Discard</button>
                            <button className="save-btn" onClick={handleSaveZone}>
                                <Check size={18} /> Create Zone
                            </button>
                        </div>
                    </div>
                </div>
            )}
        </div>
    );
};

export default MapContainer;
