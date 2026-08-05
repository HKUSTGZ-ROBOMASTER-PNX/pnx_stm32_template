# F407 PS2 Validation Task Packet

This packet records the selective integration of the latest official PS2
mapping into the F407-only validation image. The repository is authoritative
for code and test state. The Vault was not modified.

- Track: standard
- Branch: `feat/f407-ps2-validation`
- Parent baseline: `9f8707280d56ae1c8f8061cb9ec12e75f527d1f8`
- Official parent inspected: `upstream/main@9430817755e6c4dfd5b131959d43c2da1c05cfb8`
- Selected shared dependency: `pnx_modules@42a4b35b63ef7b8528ad756e03d17f1c09bab9af`
- Modules provenance: `fix/f407-ps2-stack`, based on `main@44ff9ab`
- Software closure: validated at the selected dependency
- Hardware closure: attended PASS on the recorded precursor artifact on 2026-08-06
- Hardware target: DAP `482752183208`, physical UART2 / MCU USART1
- Publication: modules feature branch pushed; parent `F4_version` push authorized
