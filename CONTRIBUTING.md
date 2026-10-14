# Como contribuir

Obrigado pelo interesse em contribuir com o **Monitor de Energia UPS**! Este é um
projeto pessoal, mas sugestões, correções e melhorias são muito bem-vindas.

## Antes de abrir uma issue

- Verifique se já existe uma issue semelhante em aberto.
- Consulte o [Manual](docs/MANUAL.md) e a
  [solução de problemas](README.md#solução-de-problemas).
- Para bugs, inclua o máximo de contexto possível (veja o template de issue).

## Fluxo de contribuição (GitHub Flow)

1. Faça um *fork* do repositório.
2. Crie um branch descritivo a partir de `main`:
   ```bash
   git checkout -b fix/descricao-curta
   # ou
   git checkout -b feat/nova-funcionalidade
   ```
3. Faça as alterações seguindo o estilo do projeto (veja abaixo).
4. Compile localmente antes de enviar:
   ```bash
   arduino-cli compile --fqbn esp32:esp32:esp32 firmware/Monitor_Energia_UPS
   ```
5. Faça *commit* com mensagem clara (Conventional Commits).
6. Envie o branch e abra um *Pull Request* explicando o que mudou e por quê.

## Estilo de código

- **Idioma:** português (pt-BR).
- **Indentação:** 2 espaços; sem tabulações.
- **Fim de linha:** LF.
- Textos da interface web ficam em `INDEX_HTML` / `CONFIG_HTML` (PROGMEM) no `.ino`.

## Testes

Não há suíte automatizada. Antes do PR:

- Compile sem *warnings* novos.
- Teste a página `/`, `/config` e `/dados`.
- Se possível, valide MQTT + auto-descoberta no Home Assistant.
- O CI (`.github/workflows/compile.yml`) compila o sketch.

## Licença

Ao contribuir, você concorda que sua contribuição será licenciada sob a
licença [MIT](LICENSE) do projeto.
