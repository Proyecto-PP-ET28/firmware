(()=>{"use strict";var e,t={306:(e,t,a)=>{const n="600",s="#fff",r="#4d4d4d",o=200,i=15e3,d=.2,l="Empuje",c="#9966ff",m=0,u=null,p="RPM",g="#ff6384",y=0,v=null,h="Tensión",E="#ffcd56",w=0,f=30,S="Corriente",x="#36a2eb",M=0,B=50;var b=a(217),I=a(993);a(54);const A={thrust:0,rpm:0,volt:0,amp:0,pwm:0,battery:0,thrustMax:0,rpmMax:0,voltMax:0,ampMax:0,lastSettingsArray:[],savedData:[]};b.Z.register(I.ZP),b.Z.defaults.color=s,b.Z.defaults.font.weight=n;const k={id:"chartAreaBorder",beforeDraw(e,t,a){const{ctx:n,chartArea:{left:s,top:r,width:o,height:i}}=e;n.save(),n.strokeStyle=a.borderColor,n.lineWidth=a.borderWidth,n.setLineDash(a.borderDash||[]),n.lineDashOffset=a.borderDashOffset,n.strokeRect(s,r,o,i),n.restore()}},L={type:"line",data:{datasets:[{label:l,borderColor:c,tension:d,data:[]}]},options:{responsive:!0,maintainAspectRatio:!1,plugins:{chartAreaBorder:{borderColor:r},legend:{display:!1}},scales:{x:{type:"realtime",realtime:{duration:i,refresh:o,delay:o,onRefresh:e=>{const t=Date.now();e.data.datasets.forEach((e=>{e.data.push({x:t,y:A.thrust})}))}},grid:{color:r}},y:{max:u,min:m,grid:{color:r}}}},plugins:[k]},T={type:"line",data:{datasets:[{label:p,borderColor:g,tension:d,data:[]}]},options:{responsive:!0,maintainAspectRatio:!1,plugins:{chartAreaBorder:{borderColor:r},legend:{display:!1}},scales:{x:{type:"realtime",realtime:{duration:i,refresh:o,delay:o,onRefresh:e=>{const t=Date.now();e.data.datasets.forEach((e=>{e.data.push({x:t,y:A.rpm})}))}},grid:{color:r}},y:{max:v,min:y,grid:{color:r}}}},plugins:[k]},F={type:"line",data:{datasets:[{label:h,borderColor:E,tension:d}]},options:{responsive:!0,maintainAspectRatio:!1,plugins:{chartAreaBorder:{borderColor:r},legend:{display:!1}},scales:{x:{type:"realtime",realtime:{duration:i,refresh:o,delay:o,onRefresh:e=>{const t=Date.now();e.data.datasets.forEach((e=>{e.data.push({x:t,y:A.volt})}))}},grid:{color:r}},y:{max:f,min:w,grid:{color:r}}}},plugins:[k]},P={type:"line",data:{datasets:[{label:S,borderColor:x,tension:d,data:[]}]},options:{responsive:!0,maintainAspectRatio:!1,plugins:{chartAreaBorder:{borderColor:r},legend:{display:!1}},scales:{x:{type:"realtime",realtime:{duration:i,refresh:o,delay:o,onRefresh:e=>{const t=Date.now();e.data.datasets.forEach((e=>{e.data.push({x:t,y:A.amp})}))}},grid:{color:r}},y:{max:B,min:M,grid:{color:r}}}},plugins:[k]};class D{static drawSliderFill(){const e=document.getElementById("slider");e.style.setProperty("--value",e.value),e.style.setProperty("--min",""==e.min?"0":e.min),e.style.setProperty("--max",""==e.max?"100":e.max)}static updateMeasurement(e){document.querySelector(`#${e}-value div`).innerText=A[e],document.querySelector(`#max-${e} div`).innerText=A[`${e}Max`]}static updateBattery(){document.getElementById("battery-level").innerText=A.battery+"%";const e=document.querySelector("#battery-icon .active");e&&e.classList.remove("active"),A.battery>=66&&document.querySelector(".battery-icon-100").classList.add("active"),A.battery<66&&A.battery>=33&&document.querySelector(".battery-icon-66").classList.add("active"),A.battery<33&&A.battery>=10&&document.querySelector(".battery-icon-33").classList.add("active"),A.battery<10&&document.querySelector(".battery-icon-0").classList.add("active")}static changeMenu(e){const t=document.querySelector(".menu-item.active");t&&t.classList.remove("active"),e.classList.add("active");const a=document.querySelector(".main-container.active");a&&a.classList.remove("active");const n=e.id.substring(0,e.id.indexOf("-"));document.getElementById(n).classList.add("active")}static updateSettings(e){document.getElementById("blades-num").value=e[0],document.getElementById("display-real-time").checked=JSON.parse(e[1]),document.getElementById("display-peak").checked=JSON.parse(e[2]),document.getElementById("pwm-min").value=parseFloat(e[3],10).toFixed(2),document.getElementById("pwm-max").value=parseFloat(e[4],10).toFixed(2),document.getElementById("current-offset").value=parseFloat(e[5],10).toFixed(3)}}const C={arePwmControlsActive:!1},O=Math.round(100*Math.random());class _{static getData(){A.thrust=1400+Math.round(20*Math.random()),A.rpm=8e3+Math.round(100*Math.random()),A.volt=(14+4*Math.random()).toFixed(2),A.amp=(14+4*Math.random()).toFixed(2),A.thrust>A.thrustMax&&(A.thrustMax=A.thrust),A.rpm>A.rpmMax&&(A.rpmMax=A.rpm),A.volt>A.voltMax&&(A.voltMax=A.volt),A.amp>A.ampMax&&(A.ampMax=A.amp)}static getBattery(){A.battery=O}}let R=!1;class q{static initAll(e){R=e,this.data(),this.battery(),!R&&this.pwm()}static data(){R?_.getData():$.getData(),D.updateMeasurement("thrust"),D.updateMeasurement("rpm"),D.updateMeasurement("volt"),D.updateMeasurement("amp"),setTimeout(q.data,400)}static pwm(){C.arePwmControlsActive||($.getPwm(),document.getElementById("slider").value=A.pwm,document.getElementById("pwm-input").value=A.pwm,D.drawSliderFill()),setTimeout(q.pwm,50)}static battery(){R?_.getBattery():$.getBattery(),D.updateBattery(),setTimeout(q.battery,2e3)}}const W=`ws://${window.location.hostname}/ws`;let N;class ${static init(){console.log("Trying to open a WebSocket connection…"),N=new WebSocket(W),N.onopen=$.onOpen,N.onclose=$.onClose,N.onmessage=$.onMessage}static getValues(){N.send("getValues")}static onOpen(){console.log("Connection opened"),$.getValues(),q.initAll()}static onClose(){console.log("Connection closed"),setTimeout($.init,2e3)}static onMessage(e){console.log(e);JSON.parse(e.data)}static sendPWM(e){N.send("PWM"+e.value.toString())}static sendString(e){N.send(e)}static getPwm(){const e=new XMLHttpRequest;e.onreadystatechange=function(){4==this.readyState&&200==this.status&&(A.pwm=parseInt(this.responseText,10))},e.open("GET","/pwm",!0),e.send()}static getData(){const e=new XMLHttpRequest;e.onreadystatechange=function(){if(4==this.readyState&&200==this.status){console.log(" this.responseText: ",this.responseText);const e=this.responseText.split(",");A.thrust=parseInt(e[0],10),A.rpm=parseInt(e[1],10),A.volt=parseFloat(e[2],10).toFixed(2),A.amp=parseFloat(e[3],10).toFixed(2),A.thrust>A.thrustMax&&(A.thrustMax=A.thrust),A.rpm>A.rpmMax&&(A.rpmMax=A.rpm),A.volt>A.voltMax&&(A.voltMax=A.volt),A.amp>A.ampMax&&(A.ampMax=A.amp)}},e.open("GET","/data",!0),e.send()}static getSettings(e){const t=new XMLHttpRequest;t.onreadystatechange=function(){if(4==this.readyState&&200==this.status){console.log(" this.responseText: ",this.responseText);const t=this.responseText.split(",");A.lastSettingsArray=t,console.log("settingsArray: ",t),D.updateSettings(t),D.changeMenu(e)}},t.open("GET","/settings",!0),t.send()}static getBattery(){const e=new XMLHttpRequest;e.onreadystatechange=function(){4==this.readyState&&200==this.status&&(A.battery=parseInt(this.responseText,10))},e.open("GET","/bat",!0),e.send()}}const V=JSON.parse("false");!function(){const e=document.getElementById("thrust-chart").getContext("2d"),t=(new b.Z(e,L),document.getElementById("rpm-chart").getContext("2d")),a=(new b.Z(t,T),document.getElementById("amp-chart").getContext("2d")),n=(new b.Z(a,P),document.getElementById("volt-chart").getContext("2d"));new b.Z(n,F)}(),class{static all(){this.pwm(),this.buttons(),this.menu(),this.keyboard(),this.settingsForm(),this.settingsBtns("pwm-min"),this.settingsBtns("pwm-max"),this.settingsBtns("current-offset")}static pwm(){const e=document.getElementById("slider"),t=document.getElementById("pwm-input"),a=document.getElementById("increase-pwm"),n=document.getElementById("decrease-pwm"),s=document.getElementById("stop");e.value=0,t.value=0,D.drawSliderFill(),[e,a,n,s].forEach((e=>{e.addEventListener("mousedown",(()=>{C.arePwmControlsActive=!0,$.sendString("S_DOWN")})),e.addEventListener("mouseup",(()=>{setTimeout((()=>{C.arePwmControlsActive=!1,$.sendString("S_UP")}),750)}))})),t.addEventListener("focusin",(()=>{C.arePwmControlsActive=!0,$.sendString("S_DOWN")})),t.addEventListener("focusout",(()=>{setTimeout((()=>{C.arePwmControlsActive=!1,$.sendString("S_UP")}),750)})),e.addEventListener("input",(a=>{t.value=a.target.value,A.pwm=a.target.value,D.drawSliderFill(),$.sendPWM(e)})),t.addEventListener("change",(t=>{e.value=t.target.value,A.pwm=t.target.value,D.drawSliderFill(),$.sendPWM(e)})),a.addEventListener("click",(()=>{let a=parseInt(e.value,10);a+=5,a>100&&(a=100),e.value=a,t.value=a,A.pwm=a,D.drawSliderFill(),$.sendPWM(e)})),n.addEventListener("click",(()=>{let a=parseInt(e.value,10);a-=5,a<0&&(a=0),e.value=a,t.value=a,A.pwm=a,D.drawSliderFill(),$.sendPWM(e)})),s.addEventListener("mousedown",(()=>{e.value=0,t.value=0,A.pwm=0,D.drawSliderFill(),$.sendPWM(e)}))}static buttons(){const e=document.getElementById("snapshot"),t=document.getElementById("reset-max"),a=document.getElementById("tare");e.addEventListener("click",(()=>{console.log("SNAP"),$.sendString("SNAP")})),a.addEventListener("click",(()=>{console.log("TARE"),$.sendString("TARE")})),t.addEventListener("click",(()=>{console.log("RST"),A.thrustMax=0,A.rpmMax=0,A.voltMax=0,A.ampMax=0,$.sendString("RST")}))}static menu(){document.querySelectorAll(".menu-item").forEach((e=>{e.addEventListener("click",(e=>{"settings-menu"===e.currentTarget.id?$.getSettings(e.currentTarget):"measurements-menu"===e.currentTarget.id?$.getSavedData(e.currentTarget):D.changeMenu(e.currentTarget)}))}))}static keyboard(){const e=document.getElementById("slider"),t=document.getElementById("pwm-input");window.addEventListener("keydown",(a=>{if("s"!==a.key&&"Escape"!==a.key&&" "!==a.key||(a.preventDefault(),e.value=0,t.value=0,A.pwm=0,D.drawSliderFill(),$.sendPWM(e)),"ArrowUp"===a.key){a.preventDefault();const e=document.querySelector(".menu-item.active");"settings-menu"===e.id?document.getElementById("measurements-menu").click():"measurements-menu"===e.id&&document.getElementById("home-menu").click()}if("ArrowDown"===a.key){a.preventDefault();const e=document.querySelector(".menu-item.active");"home-menu"===e.id?document.getElementById("measurements-menu").click():"measurements-menu"===e.id&&document.getElementById("settings-menu").click()}"ArrowLeft"===a.key&&(a.preventDefault(),document.getElementById("decrease-pwm").click()),"ArrowRight"===a.key&&(a.preventDefault(),document.getElementById("increase-pwm").click()),"c"===a.key&&(a.preventDefault(),document.getElementById("snapshot").click()),"g"===a.key&&(a.preventDefault(),document.getElementById("recording").click()),"r"===a.key&&(a.preventDefault(),document.getElementById("reset-max").click())}))}static settingsForm(){document.querySelector("form.settings").addEventListener("submit",(e=>{e.preventDefault();const t=document.getElementById("blades-num").value,a=(+document.getElementById("display-real-time").checked).toString(),n=(+document.getElementById("display-peak").checked).toString(),s=document.getElementById("pwm-min").value,r=document.getElementById("pwm-max").value,o=document.getElementById("current-offset").value;t!==A.lastSettingsArray[0]&&$.sendString(`SAVE_BLADES_NUM_${t}`),a!==A.lastSettingsArray[1]&&$.sendString(`SAVE_DISPLAY_REAL_TIME_${a}`),n!==A.lastSettingsArray[2]&&$.sendString(`SAVE_DISPLAY_PEAK_${n}`),s!==A.lastSettingsArray[3]&&$.sendString(`SAVE_PWM_MIN_${s}`),r!==A.lastSettingsArray[4]&&$.sendString(`SAVE_PWM_MAX_${r}`),o!==A.lastSettingsArray[5]&&$.sendString(`SAVE_CURRENT_OFFSET_${o}`)}))}static settingsBtns(e){const t=document.getElementById(e),a=parseFloat(t.getAttribute("min"),10),n=parseFloat(t.getAttribute("max"),10),s=parseFloat(t.getAttribute("step"),10);document.getElementById(`${e}-decrease`).addEventListener("click",(()=>{let n=parseFloat(t.value,10);n-=s,n<a&&(n=a),t.value=n.toFixed("current-offset"===e?3:2)})),document.getElementById(`${e}-increase`).addEventListener("click",(()=>{let a=parseFloat(t.value,10);a+=s,a>n&&(a=n),t.value=a.toFixed("current-offset"===e?3:2)}))}}.all(),V?q.initAll(V):window.addEventListener("load",$.init),window.addEventListener("load",(()=>{window.matchMedia("(prefers-color-scheme: dark)").addEventListener("change",(e=>{const t=document.getElementById("light-theme-icon"),a=document.getElementById("dark-theme-icon");e.matches?(t.parentNode.removeChild(t),document.head.append(a)):(a.parentNode.removeChild(a),document.head.append(t))}))}))}},a={};function n(e){var s=a[e];if(void 0!==s)return s.exports;var r=a[e]={exports:{}};return t[e](r,r.exports,n),r.exports}n.m=t,e=[],n.O=(t,a,s,r)=>{if(!a){var o=1/0;for(c=0;c<e.length;c++){for(var[a,s,r]=e[c],i=!0,d=0;d<a.length;d++)(!1&r||o>=r)&&Object.keys(n.O).every((e=>n.O[e](a[d])))?a.splice(d--,1):(i=!1,r<o&&(o=r));if(i){e.splice(c--,1);var l=s();void 0!==l&&(t=l)}}return t}r=r||0;for(var c=e.length;c>0&&e[c-1][2]>r;c--)e[c]=e[c-1];e[c]=[a,s,r]},n.d=(e,t)=>{for(var a in t)n.o(t,a)&&!n.o(e,a)&&Object.defineProperty(e,a,{enumerable:!0,get:t[a]})},n.o=(e,t)=>Object.prototype.hasOwnProperty.call(e,t),(()=>{var e={179:0};n.O.j=t=>0===e[t];var t=(t,a)=>{var s,r,[o,i,d]=a,l=0;if(o.some((t=>0!==e[t]))){for(s in i)n.o(i,s)&&(n.m[s]=i[s]);if(d)var c=d(n)}for(t&&t(a);l<o.length;l++)r=o[l],n.o(e,r)&&e[r]&&e[r][0](),e[r]=0;return n.O(c)},a=self.webpackChunkesima=self.webpackChunkesima||[];a.forEach(t.bind(null,0)),a.push=t.bind(null,a.push.bind(a))})();var s=n.O(void 0,[955],(()=>n(306)));s=n.O(s)})();