import React, { useState } from 'react';
import { X, Send, User, ChevronRight } from 'lucide-react';
import './MessageModal.css';

const MessageModal = ({ isOpen, onClose, recipient }) => {
    const [message, setMessage] = useState('');
    const [isSent, setIsSent] = useState(false);

    if (!isOpen) return null;

    const handleSend = () => {
        if (!message.trim()) return;
        setIsSent(true);
        setTimeout(() => {
            setIsSent(false);
            setMessage('');
            onClose();
        }, 1500);
    };

    return (
        <div className="modal-overlay">
            <div className="message-modal">
                <div className="modal-header">
                    <div className="header-title">
                        <Send size={20} color="var(--color-brand-green)" />
                        <h3>Secure Channel: Dispatch to Field</h3>
                    </div>
                    <button className="close-btn" onClick={onClose}><X size={20} /></button>
                </div>

                <div className="modal-body">
                    <div className="recipient-info">
                        <div className="recipient-label">RECIPIENT</div>
                        <div className="recipient-name">
                            <User size={14} />
                            <span>{recipient}</span>
                            <ChevronRight size={14} />
                            <span className="device-tag">#IND-8821</span>
                        </div>
                    </div>

                    <div className="input-container">
                        <label>TYPE YOUR MESSAGE</label>
                        <textarea
                            placeholder="Type instructions or emergency alerts here..."
                            value={message}
                            onChange={(e) => setMessage(e.target.value)}
                            autoFocus
                        />
                    </div>

                    <div className="quick-actions">
                        <button onClick={() => setMessage('Team Alpha is 5 minutes out. Stay where you are.')}>ETA UPDATE</button>
                        <button onClick={() => setMessage('Confirm your status. Are you safe to move?')}>STATUS CHECK</button>
                        <button onClick={() => setMessage('Emergency response dispatched. Help is on the way.')}>DISPATCH NOTICE</button>
                    </div>
                </div>

                <div className="modal-footer">
                    <button className="cancel-btn" onClick={onClose}>Discard</button>
                    <button
                        className={`send-btn ${isSent ? 'sent' : ''}`}
                        onClick={handleSend}
                        disabled={!message.trim() || isSent}
                    >
                        {isSent ? 'MESSAGE SENT' : (
                            <>
                                <Send size={18} /> SEND ALERT
                            </>
                        )}
                    </button>
                </div>
            </div>
        </div>
    );
};

export default MessageModal;
