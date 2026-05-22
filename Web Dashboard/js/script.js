// ==========================================
// 1. SETUP GRAFIK CHART.JS
// ==========================================
Chart.defaults.animation = false;
Chart.defaults.elements.point.radius = 0; 
Chart.defaults.elements.line.borderWidth = 2; 

const customLegendStyle = {
    display: true,
    position: 'bottom',
    labels: {
        useBorderRadius: true,
        borderRadius: 4,      
        boxWidth: 32,         
        boxHeight: 14,        
        padding: 20,          
        font: { size: 14, weight: '500', family: "'Montserrat', sans-serif" },
        color: '#333'
    }
};

const commonOptions = {
    responsive: true, 
    maintainAspectRatio: false, 
    animation: false, 
    normalized: true, 
    spanGaps: true,
    scales: { x: { display: false } },
    plugins: { legend: customLegendStyle }
};

const ctxAccel = document.getElementById('chartAccel').getContext('2d');
const chartAccel = new Chart(ctxAccel, {
    type: 'line',
    data: {
        labels: [],
        datasets: [
            { label: 'Acc X', data: [], borderColor: '#ffc107', backgroundColor: 'transparent', tension: 0.1 },
            { label: 'Acc Y', data: [], borderColor: '#ff5722', backgroundColor: 'transparent', tension: 0.1 },
            { label: 'Acc Z', data: [], borderColor: '#e91e63', backgroundColor: 'transparent', tension: 0.1 }
        ]
    },
    options: { ...commonOptions, scales: { ...commonOptions.scales, y: { suggestedMin: -20, suggestedMax: 20 } } }
});

const ctxGyro = document.getElementById('chartGyro').getContext('2d');
const chartGyro = new Chart(ctxGyro, {
    type: 'line',
    data: {
        labels: [],
        datasets: [
            { label: 'Gyro X', data: [], borderColor: '#00bcd4', backgroundColor: 'transparent', tension: 0.1 },
            { label: 'Gyro Y', data: [], borderColor: '#3f51b5', backgroundColor: 'transparent', tension: 0.1 },
            { label: 'Gyro Z', data: [], borderColor: '#4caf50', backgroundColor: 'transparent', tension: 0.1 }
        ]
    },
    options: { ...commonOptions, scales: { ...commonOptions.scales, y: { suggestedMin: -100, suggestedMax: 100 } } }
});

const MAX_CHART_POINTS = 75;

// ==========================================
// 2. LOGIKA KONTROL CSV
// ==========================================
let isRecording = false;
let csvDataArray = [];
let dataCount = 0;

const btnStart = document.getElementById('btn-start');
const btnStop = document.getElementById('btn-stop');
const btnDownload = document.getElementById('btn-download');
const recordStatus = document.getElementById('record-status');

btnStart.addEventListener('click', () => {
    isRecording = true;
    csvDataArray = ["Timestamp(ms),AccX,AccY,AccZ,GyroX,GyroY,GyroZ,Roll,Pitch,Yaw"]; 
    dataCount = 0;
    
    btnStart.disabled = true;
    btnStop.disabled = false;
    btnDownload.style.display = "none";
    recordStatus.innerText = "Merekam (100Hz)... 0 baris";
    recordStatus.style.color = "#d32f2f";
});

btnStop.addEventListener('click', () => {
    isRecording = false;
    
    btnStart.disabled = false;
    btnStop.disabled = true;
    recordStatus.innerText = `Selesai! Total: ${dataCount} baris.`;
    recordStatus.style.color = "#2e7d32";

    const blob = new Blob([csvDataArray.join("\n")], { type: 'text/csv' });
    const url = window.URL.createObjectURL(blob);
    
    btnDownload.href = url;
    btnDownload.download = `Log_AI_WISE_${Date.now()}.csv`;
    btnDownload.style.display = "inline-block";
});

// ==========================================
// 3. PENERIMAAN DATA DARI ESP32
// ==========================================
const gateway = "http://172.20.10.2/events"; 
let latestData = null;

if (!!window.EventSource) {
    const source = new EventSource(gateway);
    const wsStatus = document.getElementById('ws-status');

    source.addEventListener('open', () => {
        wsStatus.innerText = 'TERHUBUNG';
        wsStatus.className = 'connection-status connected';
    });

    source.addEventListener('error', (e) => {
        if (e.target.readyState != EventSource.OPEN) {
            wsStatus.innerText = 'TERPUTUS';
            wsStatus.className = 'connection-status disconnected';
        }
    });

    source.addEventListener('sensor_data', (e) => {
        try {
            const data = JSON.parse(e.data);
            
            latestData = data;

            if (isRecording) {
                const row = `${data.time || 0},${data.x},${data.y},${data.z},${data.gx || 0},${data.gy || 0},${data.gz || 0},${data.roll},${data.pitch},${data.yaw}`;
                csvDataArray.push(row);
                dataCount++;
            }
        } catch (error) {}
    });
} else {
    console.error("Browser Anda tidak mendukung Server-Sent Events (SSE).");
}

// ==========================================
// 4. PEMBARUAN LAYAR DOM & GRAFIK
// ==========================================
setInterval(() => {
    if (latestData) {
        document.getElementById('val-total-accel').innerText = parseFloat(latestData.total || 0).toFixed(2);
        document.getElementById('val-roll').innerText = parseFloat(latestData.roll).toFixed(1);
        document.getElementById('val-pitch').innerText = parseFloat(latestData.pitch).toFixed(1);
        document.getElementById('val-yaw').innerText = parseFloat(latestData.yaw).toFixed(1);

        const timeString = latestData.time.toString();
        
        chartAccel.data.labels.push(timeString);
        chartAccel.data.datasets[0].data.push(latestData.x);
        chartAccel.data.datasets[1].data.push(latestData.y);
        chartAccel.data.datasets[2].data.push(latestData.z);

        chartGyro.data.labels.push(timeString);
        chartGyro.data.datasets[0].data.push(latestData.gx || 0);
        chartGyro.data.datasets[1].data.push(latestData.gy || 0);
        chartGyro.data.datasets[2].data.push(latestData.gz || 0);

        if (chartAccel.data.labels.length > MAX_CHART_POINTS) {
            chartAccel.data.labels.shift();
            chartAccel.data.datasets.forEach(ds => ds.data.shift());
            
            chartGyro.data.labels.shift();
            chartGyro.data.datasets.forEach(ds => ds.data.shift());
        }

        chartAccel.update();
        chartGyro.update();

        if (isRecording) {
            recordStatus.innerText = `Merekam (100Hz)... ${dataCount} baris`;
        }
    }
}, 66);