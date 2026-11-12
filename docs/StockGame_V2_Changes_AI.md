# StockGame V2 변경 이력

비교 기준은 제출용 Version 1로 지정한 직전 완성본이다. V2는 일반용에 restart 확인 명령을 추가했다. 보고서에 표시한 버전은 프로젝트 버전과 파일 변경 여부를 구분한다.

사용 명령은 restart, restart yes, restart no이다. 확인 후 복구 기록에 성공하면 새 게임을 반영한다. 본 저장은 q에서 갱신한다. 저장 파일명 game_v4.sav는 기존 바이너리 형식과의 호환을 위해 유지한다.

## 헤더 파일

- presentation/presentation.h (Ver 1.0). 시연 명령과 시장·주문 사건 재현 함수 선언. V2에서 변경 사항 없음.
- shared/include/common.h (Ver 1.0). 표준 입출력·정수·문자열 등 공통 선언. V2에서 변경 사항 없음.
- shared/include/company.h (Ver 1.0). 종목 초기화·회사 ID 조회·신규 상장·재무 갱신 함수 선언. V2에서 변경 사항 없음.
- shared/include/gameConfig.h (Ver 1.0). 종목 수·갱신 주기·거래 비용 등 규칙 상수. V2에서 변경 사항 없음.
- shared/include/gameControl.h (Ver 1.0). 한 회차의 시장·재무·가격·폐지·체결·기록 순서 조정 함수 선언. V2에서 변경 사항 없음.
- shared/include/gameData.h (Ver 1.0). Game·Company·Order 등 공통 자료형. V2에서 변경 사항 없음.
- shared/include/history.h (Ver 1.0). 가격·거래·자산 이력 관리 함수 선언. V2에서 변경 사항 없음.
- shared/include/input.h (Ver 1.0). 문자 단위 입력, 입력 줄과 숫자 인수 확인. V2에서 변경 사항 없음.
- shared/include/listing.h (Ver 1.0). 상장폐지·관련 자산과 주문 정리·신규 상장 함수 선언. V2에서 변경 사항 없음.
- shared/include/market.h (Ver 1.0). 공포지수, 장세와 남은 기간 관리 함수 선언. V2에서 변경 사항 없음.
- shared/include/order.h (Ver 1.0). 시장가·지정가 주문 접수, 예약, 취소, 체결 조건 확인 함수 선언. V2에서 변경 사항 없음.
- shared/include/portfolio.h (Ver 1.0). 총자산·파산·사용 가능 자산·비용과 원가 계산 함수 선언. V2에서 변경 사항 없음.
- shared/include/random.h (Ver 1.0). 게임 상태를 이용한 범위 난수 생성 함수 선언. V2에서 변경 사항 없음.
- shared/include/saveLoad.h (Ver 1.0). 바이너리 저장·검증·최신 유효 상태 복원 함수 선언. V2에서 변경 사항 없음.
- shared/include/screenOut.h (Ver 1.0). 콘솔 화면 구성, 변경 행 갱신과 금액 표기. V2에서 변경 사항 없음.
- shared/include/session.h (Ver 2.0). 초기 실행·시간 계산·명령 분배·복구 기록 요청. 일반용 다시하기 활성 여부와 확인 대기 여부를 Session에 추가.
- shared/include/stockPrice.h (Ver 1.0). 시장·업종·재무·난수를 이용한 주가 계산 함수 선언. V2에서 변경 사항 없음.
- shared/include/stockQuery.h (Ver 1.0). 종목 검색·정렬·관심 종목 관리 함수 선언. V2에서 변경 사항 없음.
- shared/include/trade.h (Ver 1.0). 체결 확정 시 현금·수량·원가·손익 반영 함수 선언. V2에서 변경 사항 없음.

## 소스 파일

- presentation/main.c (Ver 2.0). 시연용 실행 순서. 시연용 제목 및 실행 도움말 버전 표기만 V2로 변경. 시연 동작은 동일.
- presentation/presentation.c (Ver 1.0). 시연 명령과 시장·주문 사건 재현. V2에서 변경 사항 없음.
- production/main.c (Ver 2.0). 일반용 실행 순서. 다시하기 활성화, 도움말과 확인 안내 표시, 잘못된 입력에서 확인 취소. 일반용 제목을 V2로 표기.
- shared/src/company.c (Ver 1.0). 종목 초기화·회사 ID 조회·신규 상장·재무 갱신. V2에서 변경 사항 없음.
- shared/src/gameControl.c (Ver 2.0). 한 회차의 시장·재무·가격·폐지·체결·기록 순서 조정. 주석 보강 · 기능 변경 없음. listing.c, order.c, company.c, history.c의 호출 관계를 표시.
- shared/src/history.c (Ver 1.0). 가격·거래·자산 이력 관리. V2에서 변경 사항 없음.
- shared/src/input.c (Ver 1.0). 문자 단위 입력, 입력 줄과 숫자 인수 확인. V2에서 변경 사항 없음.
- shared/src/listing.c (Ver 1.0). 상장폐지·관련 자산과 주문 정리·신규 상장. V2에서 변경 사항 없음.
- shared/src/market.c (Ver 1.0). 공포지수, 장세와 남은 기간 관리. V2에서 변경 사항 없음.
- shared/src/order.c (Ver 1.0). 시장가·지정가 주문 접수, 예약, 취소, 체결 조건 확인. V2에서 변경 사항 없음.
- shared/src/portfolio.c (Ver 1.0). 총자산·파산·사용 가능 자산·비용과 원가 계산. V2에서 변경 사항 없음.
- shared/src/random.c (Ver 1.0). 게임 상태를 이용한 범위 난수 생성. V2에서 변경 사항 없음.
- shared/src/saveLoad.c (Ver 1.0). 바이너리 저장·검증·최신 유효 상태 복원. V2에서 변경 사항 없음.
- shared/src/screenOut.c (Ver 2.0). 콘솔 화면 구성, 변경 행 갱신과 금액 표기. 기본 화면 제목의 버전 표기만 V2로 변경.
- shared/src/session.c (Ver 2.0). 초기 실행·시간 계산·명령 분배·복구 기록 요청. 다시하기 요청·확인·취소 처리, 새 상태의 복구 기록 성공 후 교체. 주문 명령 검사를 execute_order로 분리하고 연결 모듈 주석을 보강.
- shared/src/stockPrice.c (Ver 1.0). 시장·업종·재무·난수를 이용한 주가 계산. V2에서 변경 사항 없음.
- shared/src/stockQuery.c (Ver 1.0). 종목 검색·정렬·관심 종목 관리. V2에서 변경 사항 없음.
- shared/src/trade.c (Ver 1.0). 체결 확정 시 현금·수량·원가·손익 반영. V2에서 변경 사항 없음.
- tests/test_game.c (Ver 2.0). 거래·시장·저장·콘솔 회귀 검사. 콘솔 화면 검사에서 확인할 버전 문자열을 V2로 변경.
