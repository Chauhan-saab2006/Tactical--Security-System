import os
import sqlite3
import time
from datetime import datetime


def resolve_db_path() -> str:
    # backend/tools/live_signal_watch.py -> backend/sql_app.db
    backend_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    return os.path.join(backend_dir, "sql_app.db")


def wait_for_db(db_path: str) -> None:
    while not os.path.exists(db_path):
        print(f"[watch] waiting for DB: {db_path}")
        time.sleep(1)


def main() -> None:
    db_path = resolve_db_path()
    wait_for_db(db_path)
    print(f"[watch] connected to {db_path}")
    print("[watch] listening for incoming telemetry...")

    last_id = 0
    conn = sqlite3.connect(db_path)
    conn.row_factory = sqlite3.Row

    try:
        while True:
            rows = conn.execute(
                """
                SELECT id, device_id, latitude, longitude, pulse, sos, received_at
                FROM signals
                WHERE id > ?
                ORDER BY id ASC
                """,
                (last_id,),
            ).fetchall()

            for row in rows:
                last_id = row["id"]
                ts = row["received_at"]
                if ts is None:
                    ts = datetime.utcnow().isoformat()
                print(
                    f"[signal #{row['id']}] device={row['device_id']} "
                    f"lat={row['latitude']:.6f} lon={row['longitude']:.6f} "
                    f"pulse={row['pulse']} sos={bool(row['sos'])} ts={ts}"
                )

            time.sleep(1)
    except KeyboardInterrupt:
        print("\n[watch] stopped")
    finally:
        conn.close()


if __name__ == "__main__":
    main()

