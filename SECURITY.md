# Política de segurança

## Escopo

Firmware para **ESP32** que monitora tensão AC (ZMPT101B) e DC (INA219),
com display local, interface web e MQTT. Esta política cobre o código deste repositório.

## Versões suportadas

Apenas a versão mais recente (`main` / última *release*) recebe correções.

| Versão | Suportada |
|--------|-----------|
| 1.0.x  | ✅        |
| < 1.0  | ❌        |

## Relatando uma vulnerabilidade

Por favor **não abra uma issue pública** para vulnerabilidades.
Use **Security → Report a vulnerability** no GitHub ou contate o mantenedor.

Inclua: descrição, impacto, passos para reproduzir, versão/placa/ambiente.

## Boas práticas de implantação

- **Troque `admin/admin`** da página `/config` antes de colocar em produção.
  O sistema usa **HTTP sem HTTPS** — use apenas em rede local confiável.
- Mantenha o ESP32 em rede Wi-Fi confiável; `/` e `/dados` não exigem login.
- Prefira broker MQTT com autenticação.
- **Alta tensão:** o ZMPT101B mede 110/220 V AC. Trabalhe desenergizado e
  siga a NBR 5410. Projeto sem certificação elétrica — use por sua conta e risco.

## Segredos no repositório

Este repositório **não deve conter** credenciais reais. Wi-Fi e MQTT são
configurados em tempo de execução (portal cativo / `/config`) e gravados
apenas na NVS. Nunca faça *commit* de senhas ou tokens.
