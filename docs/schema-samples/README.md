# 예시 스키마

- `semiconductor_sample.json` — `[CRA_AI] Day3_개인과제_반도체시료관리.pdf`의 "시료 관리 > 시료
  등록" 속성값(시료 ID, 이름, 평균 생산시간, 수율)을 그대로 반영한 도메인 예시. 추가로
  `status`(주문 상태), `registeredAt`(등록일), `note`(nullable 문자열), `tags`(배열),
  `customer`(중첩 객체)로 라이브러리가 지원하는 모든 타입을 한 번씩 보여준다.
- `user_profile.json` — 반도체 도메인과 무관한 범용 예시. 이메일 형태를 정규식으로 표현하고,
  배열 원소로 `enum` 타입을 사용하는 등 다른 프로젝트에서 바로 참고할 수 있는 패턴을 담았다.

## 실행 예시

```
DummyDataGenerator.exe docs\schema-samples\semiconductor_sample.json 10
DummyDataGenerator.exe docs\schema-samples\user_profile.json 5 users.json 2026
```
