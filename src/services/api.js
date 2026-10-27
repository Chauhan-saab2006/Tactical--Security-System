const API_BASE_URL = import.meta.env.VITE_API_URL || 'https://lifeline-production-1041.up.railway.app/api/v1';
export const fetchActiveSOS = async () => {
    const response = await fetch(`${API_BASE_URL}/sos/active`);
    if (!response.ok) throw new Error('Failed to fetch active SOS events');
    return response.json();
};

export const resolveSOS = async (eventId) => {
    const response = await fetch(`${API_BASE_URL}/sos/resolve/${eventId}`, {
        method: 'POST'
    });
    if (!response.ok) throw new Error('Failed to resolve SOS event');
    return response.json();
};

export const login = async (email, password) => {
    const formData = new FormData();
    formData.append('username', email);
    formData.append('password', password);

    const response = await fetch(`${API_BASE_URL}/login`, {
        method: 'POST',
        body: formData
    });
    if (!response.ok) throw new Error('Login failed');
    return response.json();
};

export const fetchDevicesStatus = async () => {
    const response = await fetch(`${API_BASE_URL}/devices/status`);
    if (!response.ok) throw new Error('Failed to fetch devices status');
    return response.json();
};
