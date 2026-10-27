import React, { useState, useEffect } from 'react';
import { User, Search, RefreshCw, Hash, X } from 'lucide-react';
import './UsersPanel.css';

const UsersPanel = ({ onClose }) => {
    const [users, setUsers] = useState([]);
    const [loading, setLoading] = useState(true);
    const [searchQuery, setSearchQuery] = useState('');

    const fetchUsers = async () => {
        setLoading(true);
        try {
            const response = await fetch('https://lifeline-production-1041.up.railway.app/api/users');
            const data = await response.json();
            setUsers(data);
        } catch (error) {
            console.error('Error fetching users:', error);
        } finally {
            setLoading(false);
        }
    };

    useEffect(() => {
        fetchUsers();
    }, []);

    const filteredUsers = users.filter(user =>
        user.name.toLowerCase().includes(searchQuery.toLowerCase()) ||
        user.userId.toLowerCase().includes(searchQuery.toLowerCase())
    );

    return (
        <div className="users-panel animate-slide-in">
            <header className="panel-header">
                <div className="header-flex">
                    <h1>Users Management</h1>
                    <div className="header-actions">
                        <button className="refresh-btn" onClick={fetchUsers} disabled={loading}>
                            <RefreshCw size={18} className={loading ? 'spinning' : ''} />
                        </button>
                        <button className="panel-close-btn" onClick={onClose} title="Close Panel">
                            <X size={20} />
                        </button>
                    </div>
                </div>
                <p>System users registered in the central MongoDB database.</p>
            </header>

            <div className="search-bar-container">
                <div className="search-input-wrapper">
                    <Search size={18} />
                    <input
                        type="text"
                        placeholder="Search by name or ID..."
                        value={searchQuery}
                        onChange={(e) => setSearchQuery(e.target.value)}
                    />
                </div>
            </div>

            <div className="users-list">
                {loading ? (
                    <div className="loading-state">
                        <div className="loader"></div>
                        <p>Fetching user records...</p>
                    </div>
                ) : filteredUsers.length > 0 ? (
                    filteredUsers.map(user => (
                        <div key={user._id} className="user-record-card">
                            <div className="user-avatar">
                                <User size={24} />
                            </div>
                            <div className="user-details">
                                <h3>{user.name}</h3>
                                <div className="user-meta">
                                    <Hash size={12} />
                                    <span>{user.userId}</span>
                                </div>
                            </div>
                        </div>
                    ))
                ) : (
                    <div className="empty-state">
                        <p>No users found matching your search.</p>
                    </div>
                )}
            </div>
        </div>
    );
};

export default UsersPanel;
