# Preview local da interface web

Arquivos extraídos **byte a byte** do firmware (`INDEX_HTML` / `CONFIG_HTML`
em `firmware/Monitor_Energia_UPS/Monitor_Energia_UPS.ino`).

- [`index.html`](index.html) — página `/` (mock de `/dados`: AC ~127 V, DC ~12,6 V, alterna OK/OFF a cada 8 s).
- [`config.html`](config.html) — página `/config` com valores padrão preenchidos; salvar/reiniciar desativados.

Abra `index.html` no navegador. Nada aqui fala com hardware real.
