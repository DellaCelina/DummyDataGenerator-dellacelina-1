# 사용법 (USAGE)

## 1. 빌드

Visual Studio 2022(이상)에서 `DummyDataGenerator.slnx`를 열고 빌드한다. C++20, MSVC
`v145` 툴셋을 사용하며 외부 패키지 의존성은 없다.

솔루션에는 두 개의 프로젝트가 있다.

- `DummyDataGenerator` — 라이브러리 소스 + CLI/데모 실행 파일
- `DummyDataGeneratorTests` — 단위 테스트 실행 파일 (콘솔에서 PASS/FAIL 요약 출력)

## 2. 데모 실행

인자 없이 실행하면 반도체 시료 스키마로 예시 5건을 생성해 콘솔에 출력한다.

```
DummyDataGenerator.exe
```

## 3. CLI 사용법

```
DummyDataGenerator.exe <schema.json> [count] [output.json] [seed]
```

| 인자 | 필수 | 기본값 | 설명 |
|---|---|---|---|
| `schema.json` | O | - | 스키마 JSON 파일 경로 |
| `count` | X | 5 | 생성할 레코드 수 |
| `output.json` | X | (표준 출력) | 결과를 저장할 파일 경로 |
| `seed` | X | 랜덤 | 재현 가능한 결과를 위한 시드 값 |

예:

```
DummyDataGenerator.exe docs\schema-samples\semiconductor_sample.json 20 out.json 1234
```

## 4. 스키마 작성법

```json
{
  "name": "선택적 이름",
  "fields": [
    { "name": "필드명", "type": "타입", "...": "속성" }
  ]
}
```

지원 타입과 속성은 `docs/REQUIREMENTS.md`의 "FR-2. 타입별 속성" 표를 참고한다.
예시 스키마는 `docs/schema-samples/`에 있다.

- `semiconductor_sample.json` — PDF의 "시료 등록" 속성(ID/이름/평균생산시간/수율)을 반영한
  도메인 예시. `docs/schema-samples/README.md`에 필드별 설명이 있다.
- `user_profile.json` — 반도체 도메인과 무관한 범용 예시(사용자 프로필). 이 라이브러리가
  특정 도메인에 종속되지 않았음을 보여주기 위한 샘플이다.

## 5. 정규 표현식(`pattern`) 문법 요약

지원:

- 리터럴 문자, `.`(임의의 출력 가능한 ASCII 문자)
- 문자 클래스 `[abc]`, `[a-z]`, `[^a-z]`
- 숏핸드 `\d \D \w \W \s \S`
- 그룹 `(...)`, 비캡처 그룹 `(?:...)`
- 교대 `a|b|c`
- 반복자 `* + ? {m} {m,} {m,n}`
- 이스케이프 `\. \\ \( \) \[ \] \{ \} \| \^ \$ \+ \* \? \- \n \t \r`

미지원(사용 시 `RegexParseException`):

- 역참조(`\1`), lookaround(`(?=...)`, `(?!...)`)
- 문자 클래스 안의 부정 숏핸드(`[\D]`, `[\W]`, `[\S]`) — 클래스 밖에서는 지원
- 앵커(`^`, `$`)는 파싱은 되지만 아무 의미 없이 무시된다 (생성 결과는 항상 패턴 전체와
  일치하는 완전한 문자열이기 때문)

무한 반복(`*`, `+`, `{m,}`)은 기본적으로 최대 6회로 제한된다. 필드에 `"regexMaxRepeat": N`을
추가하면 상한을 바꿀 수 있다.

## 6. 커스텀 필드 타입 등록 (C++ 코드에서)

```cpp
#include "ddg/DataGenerator.h"

ddg::Schema schema = ddg::Schema::FromFile("my_schema.json");
ddg::DataGenerator generator(schema);

// "uuid"라는 필드 타입을 프로젝트에서 새로 정의
generator.GetRegistry().Register("uuid", [](const ddg::FieldSchema&,
                                             ddg::RandomEngine& rng,
                                             const ddg::GeneratorRegistry&) {
    return ddg::json::JsonValue(
        ddg::RegexStringGenerator::GenerateFromPattern(
            "[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}", rng));
});

ddg::json::JsonValue records = generator.GenerateMany(100);
```

이렇게 등록한 타입은 스키마 JSON에서 `"type": "uuid"`로 바로 사용할 수 있다.

## 7. 단위 테스트 실행

`DummyDataGeneratorTests` 프로젝트를 빌드하고 실행하면 등록된 모든 테스트가 실행되고
`[PASS]`/`[FAIL]` 요약과 함께 실패 시 0이 아닌 종료 코드를 반환한다.

```
DummyDataGeneratorTests.exe
```
