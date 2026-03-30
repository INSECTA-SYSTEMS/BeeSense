# BeeSense Dashboard

Das Dashboard ist die Weboberfläche des BeeSense-Systems.
Es zeigt Live-Daten (Ein-/Ausflüge, Temperatur, Feuchte), historische Werte und Standortinformationen an.

## Online erreichbar

Produktive URL:

`http://3.75.94.127:8080`

## Architektur

- Frontend: `index.html` + `style.css` (Chart.js, Leaflet)
- Backend: `server.js` (Node.js HTTP-Server)
- Persistenz: SQLite über `database.js`
- Live-Updates: Server-Sent Events über `/api/events`

## Funktionen

- **Übersicht:** Einflug, Ausflug, Gesamtflüge, Temperatur, Luftfeuchte
- **Flüge:** Tageswerte + Diagramm der letzten 7 Tage
- **Temperatur/Feuchte:** aktuelle Werte + 7-Tage-Verlauf
- **Standort:** bis zu 5 Stöcke mit Name/Koordinaten, Speicherung per `localStorage`
- **Einstellungen:** Theme-Umschaltung (hell/dunkel)

## API-Endpunkte

- `POST /api/tracking` – Einflug/Ausflug vom ESP32
- `POST /api/sensors` – Temperatur/Feuchte
- `GET /api/tracking` – aktueller Tracking-Stand
- `GET /api/sensors` – aktuelle Sensordaten
- `GET /api/data` – kombinierte Live-Daten
- `GET /api/history?days=7` – historische Daten aus SQLite
- `GET /api/events` – Live-Stream (SSE)

## Lokal starten

```powershell
cd dashboard
npm install
npm start
```

Dashboard: `http://localhost:8085`

## Relevante Dateien

- `index.html` – UI, Charts, EventSource-Client
- `style.css` – Layout/Theming
- `server.js` – REST/SSE + statische Dateien
- `database.js` – SQLite-Schema und Queries
- `deploy-aws.ps1` / `deploy-aws.sh` – Deployment-Skripte

## AWS-Betrieb

Für Deployment und Betrieb siehe:

- `../AWS_DEPLOYMENT_GUIDE.md`
- `DATABASE_README.md`