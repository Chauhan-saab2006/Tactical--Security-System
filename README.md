# SOS Webpage - Running Guide

This guide contains the commands to run all components of the system and enable local network access (port sharing).

## 1. Local Network Access (One-time Setup)
To allow other systems on your network to access the webpage, you must allow the ports through your firewall. 

### Option A: Using PowerShell (Recommended)
Open **PowerShell as Administrator** and run:
```powershell
New-NetFirewallRule -DisplayName " SOS Ports" -Direction Inbound -LocalPort 5173,5174,8000,5000 -Protocol TCP -Action Allow
```

### Option B: Using Command Prompt (CMD)
Open **Command Prompt as Administrator** and run this EXACT line:
```cmd
powershell -Command "New-NetFirewallRule -DisplayName 'Antigravity SOS Ports' -Direction Inbound -LocalPort 5173,5174,8000,5000 -Protocol TCP -Action Allow"
```

## 2. Running the Backend (FastAPI)
The backend handles SOS data and telemetries.

**Steps:**
1. Open a terminal in the `backend` folder.
2. Activate the virtual environment (runs from the root folder):
   ```powershell
   ..\.venv\Scripts\activate
   ```
3. Start the server:
   ```powershell
   python -m uvicorn app.main:app --host 0.0.0.0 --port 8000 --reload
   ```
*Note: Using `--host 0.0.0.0` makes it accessible on your local IP.*

## 3. Running the MongoDB Server (Express)

The server handles user data and persistence.
**Command:**
```powershell
node server/index.js
```

## 4. Running the Frontend (React + Vite)
The frontend is the user interface.
**Steps:**
1. Open a terminal in the **ROOT** folder (not the `backend` folder).
2. Run the command:
   ```powershell
   npm run dev
   ```
*Note: Vite is already configured to be accessible on your network.*

## Accessing the Application
- **From this PC:** [http://localhost:5173](http://localhost:5173)
- **From other systems:** `http://10.9.6.154:5173`
  *(Note: If Vite uses port 5174, adjust accordingly)*

---
### 🔍 Configuration Check (FYI Only)
Do **NOT** run these as commands. These are just for you to verify that the `.env` file (in the root folder) contains the correct settings:

`VITE_API_URL=http://10.9.6.154:8000/api/v1`
`VITE_WS_URL=ws://10.9.6.154:8000/ws`
