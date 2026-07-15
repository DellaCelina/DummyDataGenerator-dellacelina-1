# 아키텍처 설계 (ARCHITECTURE)

## 1. 설계 목표

- **범용성**: 반도체 시료 도메인 지식이 라이브러리 코드에 스며들지 않도록 한다. 모든 도메인
  지식은 스키마(JSON)와, 필요하면 호출자가 등록하는 커스텀 생성기에만 존재한다.
- **확장성**: 새로운 필드 타입은 `IValueGenerator`를 구현하거나 람다를 등록하는 것만으로
  추가할 수 있다. 라이브러리 내부 코드를 수정할 필요가 없다.
- **의존성 최소화**: 표준 C++20 라이브러리만 사용한다 (JSON 파서, 정규식 기반 문자열 생성기를
  직접 구현). 채점/빌드 환경에 패키지 매니저가 없다고 가정한다.
- **테스트 용이성**: 시드 기반 난수 생성으로 재현 가능한 테스트를 작성할 수 있게 한다.

## 2. 레이어 구조

```
┌───────────────────────────────────────────────────────────┐
│  main.cpp (CLI / 데모)                                      │
└───────────────────────────────────────────────────────────┘
                         │ 사용
                         ▼
┌───────────────────────────────────────────────────────────┐
│  DataGenerator                                               │
│  - 스키마 1개 + RandomEngine 1개 + GeneratorRegistry 1개를     │
│    소유하는 최상위 오케스트레이터                              │
│  - unique 필드 재시도, 레코드 N개 생성, 파일 출력               │
└───────────────────────────────────────────────────────────┘
                         │ 사용
             ┌───────────┴────────────┐
             ▼                        ▼
┌─────────────────────────┐ ┌───────────────────────────────┐
│  Schema / FieldSchema     │ │  GeneratorRegistry              │
│  - 스키마 JSON 파싱/검증   │ │  - type 문자열 → IValueGenerator │
│  - 속성 접근자(GetInt 등)  │ │  - 내장 타입 등록 + 커스텀 등록   │
└─────────────────────────┘ └───────────────────────────────┘
                                        │ 소유
                                        ▼
                         ┌─────────────────────────────┐
                         │  IValueGenerator (인터페이스)   │
                         │  Int/Double/Bool/String/Enum/  │
                         │  Date/Array/Object Generator    │
                         │  + LambdaValueGenerator(확장용) │
                         └─────────────────────────────┘
                                        │ 사용(문자열 생성 시)
                                        ▼
                         ┌─────────────────────────────┐
                         │  RegexStringGenerator          │
                         │  - 정규식 서브셋 파서 + AST     │
                         │  - AST를 랜덤하게 순회해 문자열  │
                         │    생성 (std::regex의 역방향)  │
                         └─────────────────────────────┘

┌───────────────────────────────────────────────────────────┐
│  ddg::json::JsonValue / RandomEngine (기반 유틸리티)          │
│  - 의존성 없는 JSON 파서/직렬화기                              │
│  - std::mt19937_64 래퍼 (시드 기반 재현성)                     │
└───────────────────────────────────────────────────────────┘
```

## 3. 핵심 타입

### 3.1 `ddg::json::JsonValue`

- `std::variant<monostate, bool, double, std::string, vector<JsonValue>, vector<pair<string,JsonValue>>>`
  기반의 최소 JSON 값 타입.
- 객체는 `vector<pair<string, JsonValue>>`로 표현하여 **입력 순서를 보존**한다. 생성된 더미
  데이터가 스키마에 정의한 필드 순서 그대로 출력되어 가독성이 좋다.
- `Parse`/`Dump`만 제공하는 최소 구현이며, 표준 JSON 문법(문자열 이스케이프, 유니코드
  `\uXXXX`, 숫자 지수 표기 등)을 지원한다.

### 3.2 `ddg::FieldSchema`

- 필드 하나의 원본 JSON 노드를 감싸고, `GetInt`/`GetDouble`/`GetString`/`GetBool`/`Require`
  같은 타입 안전 접근자를 제공한다.
- `type` 문자열을 **하드코딩된 enum으로 제한하지 않는다.** 이는 의도적인 설계로, 호출자가
  스키마에 임의의 타입 이름(`"uuid"`, `"koreanName"` 등)을 적고 `GeneratorRegistry`에
  해당 이름의 생성기를 등록하면 그대로 동작하게 하기 위함이다.

### 3.3 `ddg::Schema`

- `fields` 배열을 `vector<FieldSchema>`로 파싱한다. 파일/문자열/`JsonValue`로부터 생성 가능.

### 3.4 `ddg::IValueGenerator` / `ddg::GeneratorRegistry`

- `IValueGenerator::Generate(field, rng, registry)`가 확장의 핵심 지점이다.
- `GeneratorRegistry`는 `unordered_map<string, shared_ptr<IValueGenerator>>`로 타입
  이름을 생성기에 매핑한다. 생성자에서 내장 8종 타입을 등록하고, 이후 `Register()`로
  덮어쓰거나 새 타입을 추가할 수 있다.
- `LambdaValueGenerator`는 `std::function`을 `IValueGenerator`로 감싸, 새 클래스를 만들
  필요 없이 람다 한 줄로 커스텀 타입을 등록할 수 있게 한다 (`main.cpp`의 `koreanName` 예시,
  `DataGeneratorTests.cpp`의 `shout` 예시 참고).
- `array`/`object` 생성기는 내부적으로 `GenerateFieldValue(field, rng, registry)` 자유
  함수를 재귀 호출한다. 이 함수가 `nullable`/`nullProbability` 처리를 한 곳에서 담당하므로,
  최상위 필드든 배열 원소든 중첩 객체 필드든 null 처리가 항상 동일하게 동작한다.

### 3.5 `ddg::RegexStringGenerator`

- `string` 타입 필드가 `pattern` 속성을 가질 때 사용된다.
- `std::regex`는 "문자열이 패턴과 일치하는가"만 검사할 수 있고 그 역(패턴을 만족하는 문자열
  생성)은 표준 라이브러리에 없기 때문에 직접 구현했다.
- 재귀 하향 파서(recursive descent parser)로 패턴을 AST로 변환한 뒤, AST를 랜덤하게
  순회하며 문자열을 만든다.
- 지원 문법: 리터럴, `.`, `[...]`/`[^...]`(범위 포함), `\d \D \w \W \s \S`, 그룹 `(...)`와
  `(?:...)`, 교대 `|`, 반복자 `* + ? {m} {m,} {m,n}`. `^`/`$`는 허용하되 무시한다(항상
  "완전 매치"에 해당하는 문자열을 만들기 때문).
- 미지원: 역참조(backreference), lookaround, POSIX 클래스. 문자 클래스 안에서의 부정
  숏핸드(`[\D]` 등)도 미지원 — 이 경우 명시적으로 예외를 던진다.
- 무한 반복(`*`, `+`, `{m,}`)은 `defaultMaxRepeat`(기본 6, 필드 속성 `regexMaxRepeat`로
  재정의 가능)으로 상한을 두어 항상 종료를 보장한다.
- `docs/schema-samples/`의 값들과 단위 테스트는 생성된 문자열을 다시 `std::regex_match`로
  검증하는 방식으로 생성기의 정확성을 교차 검증한다 (같은 패턴에 대해 "생성 → 매치"가
  항상 성립해야 한다).

### 3.6 `ddg::DataGenerator`

- 스키마 하나로부터 레코드를 반복 생성하는 최상위 API.
- `unique: true` 필드는 `Dump(-1)`로 만든 문자열을 키로 사용해 이미 나온 값과 겹치지 않을
  때까지(최대 100회) 재시도하고, 실패하면 원인을 설명하는 예외를 던진다.
- `GenerateOne`/`GenerateMany`/`GenerateToFile` 세 가지 진입점을 제공한다.

## 4. 왜 이렇게 설계했는가 (트레이드오프)

- **enum 대신 문자열 type**: 컴파일 타임 안전성은 줄어들지만, 이 프로젝트의 요구사항(범용
  구조, 프로젝트마다 다른 필드 타입)에는 런타임 확장성이 더 중요하다고 판단했다.
- **직접 구현한 JSON/정규식**: 외부 패키지 매니저(vcpkg/NuGet) 없이도 Visual Studio에서
  바로 빌드되도록 하기 위함이다. 실제 서비스라면 nlohmann/json 등을 vcpkg로 받는 편이
  합리적이지만, 이 저장소는 오프라인/그레이딩 환경에서도 즉시 빌드 가능해야 하는 PoC이다.
- **nullable 처리를 GenerateFieldValue 한 곳에 집중**: 각 Generator가 null 여부를 각자
  처리하면 배열/객체처럼 재귀적으로 필드를 생성하는 경로에서 로직이 중복되거나 어긋날
  위험이 있다. 한 곳에서만 처리해 일관성을 보장한다.
- **unique 재시도 방식**: 완전한 조합 추적(예: 남은 값의 집합을 유지) 대신 재시도+실패시
  예외를 선택했다. 구현이 단순하고, 스키마 작성자가 "이 범위/패턴으로는 충분한 고유 값을
  만들 수 없다"는 신호를 즉시 받을 수 있다는 장점이 있다.

## 5. 확장 방법 (다른 프로젝트에서 재사용하기)

1. `include/ddg`와 `src`의 핵심 파일을 프로젝트에 포함시킨다 (외부 의존성 없음).
2. 프로젝트 고유의 스키마 JSON을 작성한다 (`docs/schema-samples/user_profile.json` 참고).
3. 필요하면 `DataGenerator::GetRegistry().Register(...)`로 도메인 특화 타입(예: `"uuid"`,
   `"email"`, `"phoneNumber"`)을 추가한다.
4. `DataGenerator::GenerateMany(n)` 또는 `GenerateToFile(path, n)`으로 더미 데이터를 뽑아
   DB 시딩, 테스트 픽스처 등에 사용한다.
