# Roadmap: adsblib → Librería C madura + envío a JOSS

**Objetivo:** convertir adsblib en una librería C robusta, bien probada y con historial de desarrollo suficiente para superar el screening y la revisión de JOSS.

**Punto de partida (agosto 2026):** 2 commits, MIT license, API pública funcional, notebook de validación, sin tests automatizados, sin CI, sin evidencia de uso en investigación.

**Duración estimada:** 6–9 meses de desarrollo activo y distribuido (no concentrar todo el trabajo en pocas semanas — JOSS y los revisores miran la distribución temporal de los commits).

---

## Fase 0 (semana 1-2): Fundamentos del repo

- [x] Definir un **Statement of Need** claro en el README: qué problema resuelve adsblib, para quién (aviónica experimental, validación offline, investigación en sistemas de vigilancia ADS-B), y qué la diferencia de alternativas existentes (dump1090, pyModeS, etc. — como *encoder*, no *decoder*).
- [x] Añadir sección "Comparación con software similar" — JOSS pregunta explícitamente si existen alternativas y por qué la tuya aporta algo distinto.
- [x] Crear `CONTRIBUTING.md` (cómo compilar, cómo correr tests, guía de estilo de código, cómo reportar issues).
- [x] Crear `CODE_OF_CONDUCT.md` (opcional pero recomendado).
- [x] Revisar que `CHANGELOG.md` siga estrictamente Keep a Changelog desde el primer release real.
- [x] Configurar **GitHub Issues** y una plantilla básica de bug report / feature request.
- [x] Crear `SECURITY.md` con el alcance de uso previsto y el proceso de reporte de vulnerabilidades.
- [x] Crear `CITATION.cff` para citación académica.
- [x] Automatizar la generación de la documentación Doxygen vía GitHub Actions y publicarla en GitHub Pages en cada push a `main`, en lugar de commitear `docs/` (evita commitear artefactos generados y elimina el riesgo de contaminar `docs/` con una build local incorrecta).

## Fase 1 (mes 1-2): Suite de tests + CI

- [ ] Convertir `encoder_validation.ipynb` en tests unitarios reproducibles en C (framework tipo **Unity**, **CMocka** o **Check**), no solo notebook.
- [ ] Cobertura de tests para cada función pública: identificación, posición (CPR par/impar), velocidad, CRC24.
- [ ] Añadir **casos límite y de error** (inputs inválidos, overflow, callsigns fuera de rango) — no solo el "happy path".
- [ ] Configurar **GitHub Actions**:
  - Build en Linux y macOS (matrix build).
  - Ejecutar suite de tests en cada push/PR.
  - Linting estático (`clang-tidy`, `cppcheck`).
  - Badge de build status en el README.
- [ ] Mantener el notebook de validación contra pyModeS como **test de integración cruzada** (comparas tu encoder contra un decoder de referencia externo) — esto es una señal de calidad muy fuerte, consérvalo.
- [ ] Objetivo de cobertura de código: idealmente >80% (usa `gcov`/`lcov`, publica el badge).

## Fase 2 (mes 2-4): Robustez de la librería

- [ ] Auditoría de memoria: `valgrind` / `AddressSanitizer` / `UndefinedBehaviorSanitizer` en CI.
- [ ] Confirmar y documentar explícitamente la garantía de "no dynamic memory allocation" con tests que lo verifiquen.
- [ ] Fuzzing básico de las funciones de parsing/encoding (`libFuzzer` o `AFL++`) si el tiempo lo permite — es un plus notable para software crítico tipo aviónica.
- [ ] Versionado semántico estricto (SemVer) en los tags de release, ligado al CHANGELOG.
- [ ] Publicar releases con binarios/artefactos si aplica, o al menos tags firmados.
- [ ] Ampliar Doxygen: asegurarse de que **cada función pública, struct y enum** tiene documentación completa (parámetros, valores de retorno, ejemplos de uso).
- [ ] Añadir ejemplos de uso más allá del snippet del README: un directorio `examples/` con 2-3 programas completos (identificación, posición, velocidad).

## Fase 3 (mes 3-6): Evidencia de uso en investigación (el requisito más crítico)

Esto es lo que más te falta y lo que JOSS más escrutina. Opciones, de más a menos sólida:

- [ ] **Mejor opción:** usar adsblib en un proyecto/experimento propio documentado públicamente (ej. un repo que consuma adsblib para generar tráfico ADS-B sintético en un banco de pruebas de radio, o para validar un receptor SDR). Un "uso downstream" real, aunque sea tuyo, cuenta.
- [ ] Si tienes o consigues contacto con algún grupo de aviónica experimental, UAV/drones, o radiofrecuencia (universidad, makerspace, asociación de aeromodelismo con instrumentación), proponer que lo usen y lo citen.
- [ ] Si escribes un paper (aunque sea de otro tema) que use adsblib como parte de la metodología, puedes:
  - Subirlo como **preprint** (arXiv u otro) — JOSS lo acepta explícitamente y no cuenta como publicación previa.
  - Citar adsblib con versión/tag concreto (idealmente con DOI vía Zenodo).
- [ ] Si nada de esto es viable a corto plazo, documenta al menos un **caso de uso interno reproducible**: un README de "cómo usé adsblib para X" con datos/resultados, aunque no sea un paper formal — da algo tangible que mostrar al equipo editorial si te lo piden de forma confidencial.

## Fase 4 (mes 5-6): Empaquetado y distribución

- [ ] Facilitar la instalación: `Makefile` con targets `install`/`uninstall`, o soporte **CMake** con `find_package` (más estándar para librerías C que un Makefile a mano).
- [ ] Publicar en un gestor de paquetes si aplica al ecosistema (ej. Conan, vcpkg) — opcional, pero suma a "buenas prácticas".
- [ ] Archivar una release concreta en **Zenodo** conectando el repo (GitHub → Zenodo integration) para obtener un DOI de software — JOSS lo pide como parte del proceso de publicación final, pero puedes dejarlo preparado antes.

## Fase 5 (mes 6+): Preparar el envío

- [ ] Redactar el `paper.md` (formato JOSS: resumen, statement of need, funcionalidad, uso en investigación, referencias en BibTeX `paper.bib`).
- [ ] Revisar la checklist oficial de JOSS de "Software paper" antes de enviar (longitud, estructura, referencias).
- [ ] Pre-revisión informal: pedir a 1-2 personas (idealmente con background en C o en sistemas embebidos/aviónica) que prueben instalar y compilar la librería desde cero siguiendo solo el README, sin tu ayuda — así detectas fricciones que un revisor de JOSS también encontraría.
- [ ] Enviar la submission en la plataforma de JOSS (whedon/Open Journals) cuando:
  - Han pasado 6+ meses de desarrollo activo y distribuido desde ahora.
  - CI en verde, cobertura de tests razonable.
  - Al menos una evidencia de uso en investigación documentable.
  - Licencia, CONTRIBUTING, Statement of Need y comparación con alternativas en su sitio.

---

## Checklist resumen de gates de JOSS (no negociables)

| Requisito | Estado actual | Bloqueante |
|---|---|---|
| Licencia OSI-aprobada | ✅ MIT | No |
| 6+ meses historial activo/distribuido | ❌ 2 commits | **Sí** |
| Tests automatizados + CI | ❌ | **Sí** |
| Evidencia de uso en investigación | ❌ | **Sí** |
| Statement of Need explícito | ❌ | Sí (fácil de arreglar) |
| Comparación con software similar | ❌ | Sí (fácil de arreglar) |
| Documentación de API | ✅ Doxygen | No |
| Instalable/reproducible por terceros | Parcial | Revisar |

---

## Nota sobre ritmo
No conviertas esto en un sprint de un fin de semana. JOSS (y cualquier revisor humano) puede notar fácilmente un historial de commits "artificialmente" agrupado en pocos días tras meses de inactividad. Lo ideal es integrar el trabajo en adsblib como una actividad recurrente (ej. 1-2 commits sustanciales por semana) durante los próximos 6-9 meses, en paralelo a tu uso real del proyecto.
