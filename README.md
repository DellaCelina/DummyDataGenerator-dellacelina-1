# DummyDataGenerator

JSON 스키마로 필드의 타입/범위/정규 표현식 등을 정의하면, 그 정의에 맞는 랜덤 더미 데이터를
JSON으로 생성해 주는 범용 C++20 라이브러리 & CLI 도구입니다.

`[CRA_AI] Day3_개인과제_반도체시료관리.pdf`의 PoC 요구사항 중 **Dummy 데이터 생성 Tool**을
구현한 저장소이며, 특정 도메인(반도체 시료)에 종속되지 않고 다른 프로젝트에서도 재사용할 수
있도록 설계했습니다.

## 특징

- **스키마도 JSON, 결과도 JSON** — 필드 이름/타입/속성을 JSON으로 정의하면 그대로 JSON 더미
  데이터가 나옵니다.
- **타입별 세밀한 속성** — 숫자는 `min`/`max`, 문자열은 **정규 표현식(`pattern`)** 또는
  길이/문자집합, enum은 후보 값 목록, 날짜는 범위/포맷, 배열/객체는 재귀적으로 정의합니다.
- **확장 가능한 생성기 레지스트리** — `GeneratorRegistry::Register`로 프로젝트별 커스텀 타입
  (uuid, 이메일, 전화번호 등)을 라이브러리 수정 없이 추가할 수 있습니다.
- **자체 구현 JSON 파서 & 정규식 기반 문자열 생성기** — 외부 의존성 없이 Visual Studio에서
  바로 빌드됩니다.
- **재현 가능한 랜덤** — 시드를 지정하면 동일한 데이터를 다시 생성할 수 있습니다.

## 빠른 시작

```
DummyDataGenerator.exe                       # 내장 데모(반도체 시료 예시) 실행
DummyDataGenerator.exe schema.json 20        # schema.json으로 20건 생성해 표준출력
DummyDataGenerator.exe schema.json 20 out.json 42   # 파일로 저장, 시드 고정
```

## 문서

- [docs/REQUIREMENTS.md](docs/REQUIREMENTS.md) — 요구사항 명세
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) — 아키텍처/클래스 설계
- [docs/USAGE.md](docs/USAGE.md) — 빌드/CLI/스키마 문법/확장 방법
- [docs/schema-samples/](docs/schema-samples/) — 예시 스키마 (반도체 시료 / 범용 사용자 프로필)

## 프로젝트 구조

```
DummyDataGenerator/          라이브러리 소스 + CLI (include/ddg, src)
DummyDataGeneratorTests/     단위 테스트 (자체 구현한 경량 테스트 러너)
docs/                        요구사항, 설계 문서, 예시 스키마
```

## 빌드

Visual Studio 2022+, C++20, MSVC `v145` 툴셋. `DummyDataGenerator.slnx`를 열고 빌드하면
됩니다. 외부 패키지 의존성은 없습니다. 자세한 내용은 [docs/USAGE.md](docs/USAGE.md) 참고.
