const WS_BASE_URL = import.meta.env.VITE_WS_URL || 'wss://lifeline-production-1041.up.railway.app/ws';

export const connectSOSWebSocket = (onMessage) => {
    const socket = new WebSocket(`${WS_BASE_URL}/sos`);

    socket.onopen = () => {
        console.log('Connected to SOS WebSocket');
    };

    socket.onmessage = (event) => {
        const data = JSON.parse(event.data);
        onMessage(data);
    };

    socket.onclose = () => {
        console.log('Disconnected from SOS WebSocket');
        // Reconnect is managed by useSOSData hook with polling fallback
        // Auto-reconnect after 5 seconds
        setTimeout(() => connectSOSWebSocket(onMessage), 5000);
    };

    socket.onerror = (error) => {
        console.error('SOS WebSocket error:', error);
    };

    return socket;
};

export const connectDeviceTelemetry = (deviceId, onMessage) => {
    const socket = new WebSocket(`${WS_BASE_URL}/telemetry/${deviceId}`);

    socket.onmessage = (event) => {
        const data = JSON.parse(event.data);
        onMessage(data);
    };

    return socket;
};
