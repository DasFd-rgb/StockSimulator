# StockGame V3 변경 기록

프로그램 버전 V3와 저장 형식 번호 5를 구분한다. 모듈 버전은 이번 수정 여부에 따라 유지한다. 사용자 작성 학습 주석을 보존한다.

```json
{
  "program_version": "V3",
  "date": "2026-11-18",
  "save_format": 5,
  "readable_save_formats": [
    4,
    5
  ],
  "production_interval_ms": 10000,
  "presentation_interval_ms": 3000,
  "save_files_max": 3,
  "pending_delist_details_max": 30,
  "trade_history_max": 256,
  "ack_command": "ack",
  "modules": [
    {
      "group": "공통 헤더",
      "file": "common.h",
      "version": "V1",
      "description": "표준 입출력·정수·문자열 등 공통 선언을 정의한다.",
      "change": "없음"
    },
    {
      "group": "공통 헤더",
      "file": "company.h",
      "version": "V1",
      "description": "종목 초기화·회사 ID 조회·신규 상장·재무 갱신에 필요한 함수의 선언을 제공한다.",
      "change": "없음"
    },
    {
      "group": "공통 헤더",
      "file": "gameConfig.h",
      "version": "V3",
      "description": "종목 수·갱신 주기·거래 비용 등 규칙 상수을 정의한다.",
      "change": "일반용 10초 갱신, 단일 저장 경로, 알림 상세 30건 상수와 저장 형식 5를 정의하였다."
    },
    {
      "group": "공통 헤더",
      "file": "gameControl.h",
      "version": "V1",
      "description": "한 회차의 시장·재무·가격·폐지·체결·기록 순서 조정에 필요한 함수의 선언을 제공한다.",
      "change": "없음"
    },
    {
      "group": "공통 헤더",
      "file": "gameData.h",
      "version": "V3",
      "description": "Game·Company·Order 등 공통 자료형을 정의한다.",
      "change": "Game에 미확인 상장폐지 알림 배열, 개수, 전체 건수와 손실 합계를 추가하였다. 기존 학습용 주석을 보존하였다."
    },
    {
      "group": "공통 헤더",
      "file": "history.h",
      "version": "V3",
      "description": "가격·거래·자산 이력 관리에 필요한 함수의 선언을 제공한다.",
      "change": "history_delist_notice 선언과 관련 모듈 설명을 추가하였다."
    },
    {
      "group": "공통 헤더",
      "file": "input.h",
      "version": "V1",
      "description": "문자 단위 입력, 입력 줄과 숫자 인수 확인에 필요한 상태 자료형과 함수 선언을 제공한다.",
      "change": "없음"
    },
    {
      "group": "공통 헤더",
      "file": "listing.h",
      "version": "V1",
      "description": "상장폐지·관련 자산과 주문 정리·신규 상장에 필요한 함수의 선언을 제공한다.",
      "change": "없음"
    },
    {
      "group": "공통 헤더",
      "file": "market.h",
      "version": "V1",
      "description": "공포지수, 장세와 남은 기간 관리에 필요한 함수의 선언을 제공한다.",
      "change": "없음"
    },
    {
      "group": "공통 헤더",
      "file": "order.h",
      "version": "V1",
      "description": "시장가·지정가 주문 접수, 예약, 취소, 체결 조건 확인에 필요한 함수의 선언을 제공한다.",
      "change": "없음"
    },
    {
      "group": "공통 헤더",
      "file": "portfolio.h",
      "version": "V1",
      "description": "총자산·파산·사용 가능 자산·비용과 원가 계산에 필요한 함수의 선언을 제공한다.",
      "change": "없음"
    },
    {
      "group": "공통 헤더",
      "file": "random.h",
      "version": "V1",
      "description": "게임 상태를 이용한 범위 난수 생성에 필요한 함수의 선언을 제공한다.",
      "change": "없음"
    },
    {
      "group": "공통 헤더",
      "file": "saveLoad.h",
      "version": "V1",
      "description": "바이너리 저장·검증·최신 유효 상태 복원에 필요한 함수의 선언을 제공한다.",
      "change": "없음"
    },
    {
      "group": "공통 헤더",
      "file": "screenOut.h",
      "version": "V1",
      "description": "콘솔 화면 구성, 변경 행 갱신과 금액 표기에 필요한 상태 자료형과 함수 선언을 제공한다.",
      "change": "없음"
    },
    {
      "group": "공통 헤더",
      "file": "session.h",
      "version": "V3",
      "description": "초기 실행·시간 계산·명령 분배·복구 기록 요청에 필요한 상태 자료형과 함수 선언을 제공한다.",
      "change": "일반용 10초, 시연용 3초의 시간 단위 주석을 정정하였다."
    },
    {
      "group": "공통 헤더",
      "file": "stockPrice.h",
      "version": "V1",
      "description": "시장·업종·재무·난수를 이용한 주가 계산에 필요한 함수의 선언을 제공한다.",
      "change": "없음"
    },
    {
      "group": "공통 헤더",
      "file": "stockQuery.h",
      "version": "V1",
      "description": "종목 검색·정렬·관심 종목 관리에 필요한 함수의 선언을 제공한다.",
      "change": "없음"
    },
    {
      "group": "공통 헤더",
      "file": "trade.h",
      "version": "V1",
      "description": "체결 확정 시 현금·수량·원가·손익 반영에 필요한 함수의 선언을 제공한다.",
      "change": "없음"
    },
    {
      "group": "공통 소스",
      "file": "company.c",
      "version": "V2",
      "description": "종목 초기화·회사 ID 조회·신규 상장·재무 갱신을 담당한다.",
      "change": "없음"
    },
    {
      "group": "공통 소스",
      "file": "gameControl.c",
      "version": "V2",
      "description": "한 회차의 시장·재무·가격·폐지·체결·기록 순서 조정을 담당한다.",
      "change": "없음"
    },
    {
      "group": "공통 소스",
      "file": "history.c",
      "version": "V3",
      "description": "가격·거래·자산 이력 관리을 담당한다.",
      "change": "고정 크기 알림 배열에 최근 30건을 보관하고 전체 미확인 건수와 손실 합계를 별도로 유지한다."
    },
    {
      "group": "공통 소스",
      "file": "input.c",
      "version": "V3",
      "description": "문자 단위 입력, 입력 줄과 숫자 인수 확인을 담당한다.",
      "change": "콘솔과 파이프 입력, Windows 함수 인수와 반환값의 학습 주석을 추가하였다."
    },
    {
      "group": "공통 소스",
      "file": "listing.c",
      "version": "V1",
      "description": "상장폐지·관련 자산과 주문 정리·신규 상장을 담당한다.",
      "change": "없음"
    },
    {
      "group": "공통 소스",
      "file": "market.c",
      "version": "V1",
      "description": "공포지수, 장세와 남은 기간 관리을 담당한다.",
      "change": "없음"
    },
    {
      "group": "공통 소스",
      "file": "order.c",
      "version": "V1",
      "description": "시장가·지정가 주문 접수, 예약, 취소, 체결 조건 확인을 담당한다.",
      "change": "없음"
    },
    {
      "group": "공통 소스",
      "file": "portfolio.c",
      "version": "V1",
      "description": "총자산·파산·사용 가능 자산·비용과 원가 계산을 담당한다.",
      "change": "없음"
    },
    {
      "group": "공통 소스",
      "file": "random.c",
      "version": "V1",
      "description": "게임 상태를 이용한 범위 난수 생성을 담당한다.",
      "change": "없음"
    },
    {
      "group": "공통 소스",
      "file": "saveLoad.c",
      "version": "V3",
      "description": "바이너리 저장·검증·최신 유효 상태 복원을 담당한다.",
      "change": "알림을 포함한 저장 형식 5를 기록한다. 형식 4도 읽고 30초 대기시간을 10초 이내로 조정한다. 알림 범위 검사를 추가하였다."
    },
    {
      "group": "공통 소스",
      "file": "screenOut.c",
      "version": "V3",
      "description": "콘솔 화면 구성, 변경 행 갱신과 금액 표기을 담당한다.",
      "change": "원 단위 차트 눈금, 직전 및 구간 시작 대비 변동액과 변동률을 표시한다. 폐지 알림을 확인 전까지 우선 표시하고 소액 잔액 안내와 Windows 학습 주석을 추가하였다."
    },
    {
      "group": "공통 소스",
      "file": "session.c",
      "version": "V3",
      "description": "초기 실행·시간 계산·명령 분배·복구 기록 요청을 담당한다.",
      "change": "ack로 알림을 확인하고 저장한다. 지정가 즉시 체결과 대기 안내를 구분한다. 전달된 저장 경로로 잠금을 만들고 원래 콘솔 상태 보관 순서를 정정하였다. Windows 설명을 보완하였다."
    },
    {
      "group": "공통 소스",
      "file": "stockPrice.c",
      "version": "V3",
      "description": "시장·업종·재무·난수를 이용한 주가 계산을 담당한다.",
      "change": "최대가 제한과 원 단위 반올림을 적용한 실제 가격으로 상승·하락·보합과 표시 등락률을 계산한다."
    },
    {
      "group": "공통 소스",
      "file": "stockQuery.c",
      "version": "V1",
      "description": "종목 검색·정렬·관심 종목 관리을 담당한다.",
      "change": "없음"
    },
    {
      "group": "공통 소스",
      "file": "trade.c",
      "version": "V3",
      "description": "체결 확정 시 현금·수량·원가·손익 반영을 담당한다.",
      "change": "보유 주식이 상장폐지될 때 기존 손실 기록을 재사용하여 미확인 알림을 추가한다."
    },
    {
      "group": "일반 사용자용",
      "file": "main.c",
      "version": "V3",
      "description": "일반용의 초기화, 입력과 시간 확인, 명령 실행, 화면 갱신과 종료 흐름을 연결한다.",
      "change": "int main(void)로 단순화하고 실행 옵션을 제거하였다. TICK_MS로 10초 갱신하며 V3를 표시한다. 메모리 실패와 Sleep 설명을 보완하였다."
    },
    {
      "group": "시연용",
      "file": "main.c",
      "version": "V3",
      "description": "3초 간격 시연 실행을 시작하고 시연 명령과 일반 명령을 연결한다.",
      "change": "int main(void)로 단순화하고 실행 옵션을 제거하였다. V3를 표시하고 메모리와 시간 주석을 보완하였다. 시연 간격은 3초이다."
    },
    {
      "group": "시연용",
      "file": "presentation.c",
      "version": "V1",
      "description": "시연 명령과 시장·주문 사건 재현을 담당한다.",
      "change": "없음"
    },
    {
      "group": "시연용",
      "file": "presentation.h",
      "version": "V1",
      "description": "시연 명령과 시장·주문 사건 재현에 필요한 함수의 선언을 제공한다.",
      "change": "없음"
    },
    {
      "group": "테스트",
      "file": "test_game.c",
      "version": "V3",
      "description": "거래·시장·저장·콘솔 회귀 검사을 담당한다.",
      "change": "알림 35건의 상세 상한 30건, 전체 합계, 저장 복원과 최대가 보합 판정을 회귀 검사한다. 큰 가격차의 차트 실행 검증용 자료 생성 기능을 추가하였다."
    }
  ],
  "runtime_checks": [
    "배포용 11초 실행 후 정확히 1회 갱신, 저장 파일 최대 3개",
    "실제 구형 30초 저장 복원과 형식 5 재저장",
    "동시 보유 폐지 2건 표시, 재실행 유지, ack 저장, 거래 이력 보존",
    "1,001원에서 800만원으로 변한 차트의 가격 눈금과 변동액 표시",
    "지정가 즉시 체결과 미체결 대기 안내 구분",
    "다시하기 초기화 및 저장 복원"
  ],
  "market_summary": {
    "480": {
      "runs": 200,
      "median_return_pct": 1.67,
      "mean_delisted": 0,
      "profitable_runs": 109
    },
    "4800": {
      "runs": 200,
      "median_return_pct": 75.945,
      "mean_delisted": 1.485,
      "profitable_runs": 157
    }
  },
  "git_release": "v3.0",
  "photos_included": false
}
```
