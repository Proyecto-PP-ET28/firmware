import config from './chartsConfig';
import Chart from 'chart.js/auto';
import ChartStreaming from 'chartjs-plugin-streaming';
import 'chartjs-adapter-luxon';
import v from '../variables';

Chart.register(ChartStreaming);
Chart.defaults.color = config.fontColor;
Chart.defaults.font.weight = config.fontWeight;

const chartAreaBorder = {
  id: 'chartAreaBorder',
  beforeDraw(chart, args, options) {
    const {
      ctx,
      chartArea: { left, top, width, height },
    } = chart;
    ctx.save();
    ctx.strokeStyle = options.borderColor;
    ctx.lineWidth = options.borderWidth;
    ctx.setLineDash(options.borderDash || []);
    ctx.lineDashOffset = options.borderDashOffset;
    ctx.strokeRect(left, top, width, height);
    ctx.restore();
  },
};

const thrustConfig = {
  type: 'line',
  data: {
    datasets: [
      {
        label: config.thrustLabel,
        borderColor: config.thrustColor,
        tension: config.lineTension,
        data: [],
      },
    ],
  },
  options: {
    responsive: true,
    maintainAspectRatio: false,
    plugins: {
      chartAreaBorder: {
        borderColor: config.gridColor,
      },
      legend: {
        display: false,
      },
    },
    scales: {
      x: {
        type: 'realtime',
        realtime: {
          duration: config.displayDuration,
          refresh: config.updateInterval,
          delay: config.updateInterval,
          onRefresh: (chart) => {
            const now = Date.now();
            chart.data.datasets.forEach((dataset) => {
              dataset.data.push({
                x: now,
                y: v.thrust,
              });
            });
          },
        },
        grid: {
          color: config.gridColor,
        },
      },
      y: {
        max: config.thrustMax,
        min: config.thrustMin,
        grid: {
          color: config.gridColor,
        },
      },
    },
  },
  plugins: [chartAreaBorder],
};

const rpmConfig = {
  type: 'line',
  data: {
    datasets: [
      {
        label: config.rpmLabel,
        borderColor: config.rpmColor,
        tension: config.lineTension,
        data: [],
      },
    ],
  },
  options: {
    responsive: true,
    maintainAspectRatio: false,
    plugins: {
      chartAreaBorder: {
        borderColor: config.gridColor,
      },
      legend: {
        display: false,
      },
    },
    scales: {
      x: {
        type: 'realtime',
        realtime: {
          duration: config.displayDuration,
          refresh: config.updateInterval,
          delay: config.updateInterval,
          onRefresh: (chart) => {
            const now = Date.now();
            chart.data.datasets.forEach((dataset) => {
              dataset.data.push({
                x: now,
                y: v.rpm,
              });
            });
          },
        },
        grid: {
          color: config.gridColor,
        },
      },
      y: {
        max: config.rpmMax,
        min: config.rpmMin,
        grid: {
          color: config.gridColor,
        },
      },
    },
  },
  plugins: [chartAreaBorder],
};

const voltConfig = {
  type: 'line',
  data: {
    datasets: [
      {
        label: config.voltLabel,
        borderColor: config.voltColor,
        tension: config.lineTension,
      },
    ],
  },
  options: {
    responsive: true,
    maintainAspectRatio: false,
    plugins: {
      chartAreaBorder: {
        borderColor: config.gridColor,
      },
      legend: {
        display: false,
      },
    },
    scales: {
      x: {
        type: 'realtime',
        realtime: {
          duration: config.displayDuration,
          refresh: config.updateInterval,
          delay: config.updateInterval,
          onRefresh: (chart) => {
            const now = Date.now();
            chart.data.datasets.forEach((dataset) => {
              dataset.data.push({
                x: now,
                y: v.volt,
              });
            });
          },
        },
        grid: {
          color: config.gridColor,
        },
      },
      y: {
        max: config.voltMax,
        min: config.voltMin,
        grid: {
          color: config.gridColor,
        },
      },
    },
  },
  plugins: [chartAreaBorder],
};

const ampConfig = {
  type: 'line',
  data: {
    datasets: [
      {
        label: config.ampLabel,
        borderColor: config.ampColor,
        tension: config.lineTension,
        data: [],
      },
    ],
  },
  options: {
    responsive: true,
    maintainAspectRatio: false,
    plugins: {
      chartAreaBorder: {
        borderColor: config.gridColor,
      },
      legend: {
        display: false,
      },
    },
    scales: {
      x: {
        type: 'realtime',
        realtime: {
          duration: config.displayDuration,
          refresh: config.updateInterval,
          delay: config.updateInterval,
          onRefresh: (chart) => {
            const now = Date.now();
            chart.data.datasets.forEach((dataset) => {
              dataset.data.push({
                x: now,
                y: v.amp,
              });
            });
          },
        },
        grid: {
          color: config.gridColor,
        },
      },
      y: {
        max: config.ampMax,
        min: config.ampMin,
        grid: {
          color: config.gridColor,
        },
      },
    },
  },
  plugins: [chartAreaBorder],
};

export function createCharts() {
  const ctxThrust = document.getElementById('thrust-chart').getContext('2d');
  const thrustChart = new Chart(ctxThrust, thrustConfig);
  
  const ctxRpm = document.getElementById('rpm-chart').getContext('2d');
  const rpmChart = new Chart(ctxRpm, rpmConfig);
  
  const ctxAmp = document.getElementById('amp-chart').getContext('2d');
  const ampChart = new Chart(ctxAmp, ampConfig);
  
  const ctxVolt = document.getElementById('volt-chart').getContext('2d');
  const voltChart = new Chart(ctxVolt, voltConfig);
}