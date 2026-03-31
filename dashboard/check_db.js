const db = require("./database");
const d = new db();
setTimeout(() => {
  d.db.all("SELECT name FROM sqlite_master WHERE type='table'", (err, rows) => {
    console.log("Tables:", err ? err.message : JSON.stringify(rows));
    d.db.get("SELECT COUNT(*) as cnt FROM daily_stats", (e2, r2) => {
      console.log("daily_stats count:", e2 ? e2.message : r2.cnt);
      d.db.get("SELECT COUNT(*) as cnt FROM bee_detections", (e3, r3) => {
        console.log("bee_detections count:", e3 ? e3.message : r3.cnt);
        d.db.get("SELECT COUNT(*) as cnt FROM sensor_data", (e4, r4) => {
          console.log("sensor_data count:", e4 ? e4.message : r4.cnt);
          d.db.all("SELECT * FROM daily_stats ORDER BY date DESC LIMIT 3", (e5, r5) => {
            console.log("Recent daily_stats:", e5 ? e5.message : JSON.stringify(r5, null, 2));
            process.exit();
          });
        });
      });
    });
  });
}, 1500);
