# 요구사항 명세 (REQUIREMENTS)

## 1. 배경

본 프로젝트는 `[CRA_AI] Day3_개인과제_반도체시료관리.pdf`에서 정의된 "반도체 시료 생산주문관리 시스템"
개인과제의 PoC 항목 중 **Dummy 데이터 생성 Tool**을 구현한다.

PDF 원문 요구사항(발췌):

> **Dummy 데이터 생성 Tool**
> Test 를 위한 Dummy Data를 생성하는 도구. Dummy Data는 연결된 DB에 추가.

다만 이 저장소(`DummyDataGenerator`)는 반도체 시료 도메인에 종속되지 않고, **다른 프로젝트에서도
재사용 가능한 범용 더미 데이터 생성 라이브러리/도구**로 설계한다. "반도체 시료" 스키마는 데모 및
검증용 예시 중 하나로만 사용한다.

## 2. 범위

### 2.1 반드시 만족해야 하는 요구사항

1. **데이터 포맷은 JSON이다.**
   - 생성 결과(Dummy Data)와 입력 스키마(구조 정의) 모두 JSON을 사용한다.
2. **구조(스키마)를 JSON으로 입력받는다.**
   - 각 필드는 이름(`name`)과 타입(`type`)을 가진다.
   - 필드는 타입에 따라 다양한 "속성(attribute)"을 가질 수 있다.
3. **필드 속성에 따라 랜덤 값을 생성한다.**
   - 숫자 타입: 값의 범위(`min`/`max`)를 속성으로 지정할 수 있다.
   - 문자열 타입: 생성 가능한 값의 형태를 **정규 표현식**으로 지정할 수 있다(`pattern`).
   - 그 외에도 열거형(`enum`), 날짜(`date`), 배열(`array`), 중첩 객체(`object`), null 허용
     여부(`nullable`) 등 실무에서 자주 쓰이는 속성을 지원한다.
4. **범용적인 인터페이스/클래스 구조**
   - 특정 도메인(반도체 시료 등)에 종속된 타입이 아니라, 타입 이름과 생성기(generator)를
     매핑하는 구조로 설계하여 새 프로젝트가 자신만의 필드 타입을 추가로 등록할 수 있어야 한다.
5. **데모용 실행 경로**를 제공한다 (콘솔에서 바로 실행해 결과를 확인할 수 있어야 한다).
6. **단위 테스트(Unit Test)**를 포함한다.
7. **Visual Studio(C++20, MSVC)** 기반으로 빌드 가능해야 한다.
8. **git 저장소로 관리**하고, 지정된 GitHub 원격 저장소로 push 한다.

### 2.2 범위 밖 (Out of Scope)

- 실제 DB 연동(PoC 문서상 "연결된 DB에 추가"는 상위 프로젝트인 DataPersistence PoC의 책임이며,
  본 저장소는 그 DB에 넣을 JSON 더미 데이터를 만들어 내는 역할까지만 담당한다). 파일 출력(`--output`)
  또는 표준 출력으로 결과를 제공하고, 실제 DB 적재는 이 결과를 소비하는 쪽에서 수행한다고 가정한다.
- 완전한 PCRE/ECMAScript 정규식 문법 지원(백레퍼런스, lookaround 등은 지원하지 않음).
  자세한 지원 범위는 `docs/ARCHITECTURE.md` 및 `docs/USAGE.md` 참고.
- GUI. 콘솔 기반 CLI로 충분하다.

## 3. 기능 요구사항 상세

### FR-1. 스키마 정의

- 스키마는 아래와 같은 JSON 형태를 가진다.

```json
{
  "name": "선택적 스키마 이름",
  "fields": [
    { "name": "필드명", "type": "타입명", "...": "타입별 속성" }
  ]
}
```

- `fields`는 1개 이상의 필드를 가져야 한다.
- 각 필드는 `name`(string, 필수), `type`(string, 필수)을 가진다.
- 지원 타입: `int`/`integer`, `double`/`float`/`number`, `bool`/`boolean`, `string`,
  `enum`, `date`, `array`, `object`. 그리고 사용자가 등록한 임의의 커스텀 타입.

### FR-2. 타입별 속성

| 타입 | 속성 | 설명 |
|---|---|---|
| int | `min`, `max` | 정수 값의 범위 (기본 0~100) |
| double | `min`, `max`, `decimalPlaces` | 실수 값의 범위와 소수 자릿수 |
| bool | `trueProbability` | true가 나올 확률 (기본 0.5) |
| string (패턴 사용) | `pattern`, `regexMaxRepeat` | 생성할 문자열의 정규 표현식과, `*`/`+`/`{n,}` 같은 무한 반복의 상한 |
| string (패턴 미사용) | `minLength`, `maxLength`, `length`, `charset` | 길이 범위와 사용할 문자 집합 |
| enum | `values` | 후보 값 목록 (문자열/숫자 등 임의 JSON 값 가능) |
| date | `min`, `max`, `format` | `YYYY-MM-DD` 범위와 출력 포맷 |
| array | `items`, `minItems`, `maxItems` | 원소 스키마와 개수 범위 |
| object | `fields` | 중첩 필드 목록 (최상위 스키마와 동일한 형태) |

- 모든 필드는 공통으로 `nullable`(bool)과 `nullProbability`(0~1)를 가질 수 있다.
- 모든 필드는 공통으로 `unique`(bool)를 가질 수 있다. 하나의 생성 실행(run) 내에서 값이
  중복되지 않도록 재시도한다.

### FR-3. 랜덤 값 생성

- 시드(seed)를 지정하면 동일한 시드로 동일한 데이터가 재현되어야 한다(테스트 용이성).
- 시드를 지정하지 않으면 매 실행마다 다른 데이터를 생성해야 한다.

### FR-4. 확장성

- 새 필드 타입을 라이브러리 코드 수정 없이 등록할 수 있어야 한다
  (`GeneratorRegistry::Register`).

### FR-5. 실행

- 인자 없이 실행하면 내장된 반도체 시료 데모 스키마로 5건의 예시 데이터를 생성해 콘솔에 출력한다.
- `schema.json [count] [output.json] [seed]` 형태로 실행하면 임의의 스키마 파일로부터
  데이터를 생성해 표준 출력 또는 파일로 내보낸다.

## 4. 비기능 요구사항

- 외부 라이브러리 의존성 없이 표준 C++20만으로 빌드 가능해야 한다 (채점 환경에 인터넷/패키지
  매니저가 없을 수 있다는 가정).
- 잘못된 스키마(필수 속성 누락, `min > max` 등)는 예외를 던져 실패 원인을 명확히 알려야 한다.
- 단위 테스트는 별도 프로젝트(`DummyDataGeneratorTests`)로 분리하고, 콘솔 실행 결과로
  PASS/FAIL과 종료 코드를 제공해야 한다.

## 5. 참고: 반도체 시료 데모 스키마와의 매핑

`src/main.cpp`의 내장 데모 스키마는 PDF의 "시료 등록" 속성 값(시료 ID, 이름, 평균 생산시간, 수율)을
그대로 반영한다.

| PDF 속성 | 데모 스키마 필드 | 타입/속성 |
|---|---|---|
| 시료 ID | `sampleId` | `string`, `pattern: "S-\\d{3}"`, `unique: true` |
| 이름 | `sampleName` | `string`, `pattern`로 웨이퍼/기판 명명 규칙 표현 |
| 평균 생산시간 | `avgProductionTimeMin` | `double`, `min/max/decimalPlaces` |
| 수율 | `yield` | `double`, `min: 0.7, max: 0.99` |
