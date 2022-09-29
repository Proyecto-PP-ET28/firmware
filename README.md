
<picture>
  <source media="(prefers-color-scheme: dark)" srcset="https://i.imgur.com/kT6ZOKA.png">
  <img height=140 align="right" alt="Logo de ESIMA" src="https://i.imgur.com/QVbBpEk.png">
</picture>

# Firmware
> Este repo contiene el código completo correspondiente al proyecto ESIMA. Esto incluye la programación del μC y la interfaz web.

<br/><br/>
<img alt="Protoboard del prototipo" src="https://i.imgur.com/keabODI.jpg">

## Firmware
El firmware del microcontrolador está programado en C++ bajo el framework de Arduino. Para compilar y subir el código es necesario instalar el IDE [PlatformIO](https://platformio.org).

1.  Clonar este repositorio
```bash
git clone git@github.com:Proyecto-PP-ET28/firmware.git
```

2. Cambiar directorio
```bash
cd firmware
```

3. Compilar y subir
```bash
pio run -t upload
```
## WebServer
La interfaz web está programada con HTML, Sass y JavaScript utilizando WebPack como "Module Bundler". Una vez empaquetado, los archivos son subidos directamente a la memoria interna del microcontrolador. Este repositorio incluye una versión pre-empaquetada del WebServer. Para reconstruirlo es necesario instalar [NPM](https://www.npmjs.com).

### Sin rebuild

1.  Clonar este repositorio
```bash
git clone git@github.com:Proyecto-PP-ET28/firmware.git
```

2. Cambiar directorio
```bash
cd firmware
```

3. Cargar memoria interna
```bash
pio run -t uploadfs
```
----
### Con rebuild

1.  Clonar este repositorio
```bash
git clone git@github.com:Proyecto-PP-ET28/firmware.git
```

2. Cambiar directorio
```bash
cd firmware
```

3. Instalar dependencias y empaquetar
```bash
cd webpack && npm install && npm run build && cd ..
```

4. Cargar memoria interna
```bash
pio run -t uploadfs
```
----
### Ejecutar en local

> Cuando el servidor se ejecuta localmente, no tiene forma de acceder a los datos de los sensores por lo que genera valores aleatorios para simular este comportamiento.

1.  Clonar este repositorio
```bash
git clone git@github.com:Proyecto-PP-ET28/firmware.git
```

2. Cambiar directorio
```bash
cd firmware
```

3. Instalar dependencias y ejecutar en modo desarrollo
```bash
cd webpack && npm install && npm start && cd ..
```
> La interfaz web se aloja por defecto en http://localhost:3000
