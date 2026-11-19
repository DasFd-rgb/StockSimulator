# StockGame V1

프로젝트의 초기 버전입니다. 일반용은 30초마다, 시연용은 3초마다 가격이 갱신됩니다.

## 실행

Releases의 StockGame_V1_User.zip을 전체 해제한 다음 일반용 production/run.cmd 또는 시연용 presentation/run.cmd를 실행합니다.

소스는 StockSimulatorV1.sln으로 열거나 build.cmd로 빌드합니다. 현재 프로젝트 파일은 v143을 지정합니다. Visual Studio 2019에서 빌드할 때에는 설치된 v142로 선택을 바꿔야 합니다. build.cmd는 설치된 v142 또는 v143을 선택합니다.

## 주요 기능

- 초기 현금 1천만원, 가상 종목 30개와 200가지 회사명 후보
- 공포지수와 시장 상태, 업종 및 재무 상태가 반영되는 가격 변화
- 시장가·지정가 주문, 미체결 취소, 수수료와 세금, 손익 및 거래 기록
- 회사 상세, 검색·정렬·관심 종목, 최근 가격의 문자 차트
- 1천원 이하 상장폐지, 보유 주식 소멸과 새 회사 상장
- 본 저장 1개와 복구 파일 2개, 자동 이어 하기
- 시연용 pause, resume, step, event, reset

이 초기 버전은 일반용 다시하기, 보유 종목 폐지의 미확인 알림, 가격 축 눈금이 추가되기 전의 코드입니다. 저장 위치는 data/game_v4.sav이고 바이너리 형식 번호는 4입니다. 프로그램 V1 표기와 저장 형식 번호는 서로 구분합니다.

## 파일별 모듈

모든 모듈의 초기 버전은 V1입니다. 최초 등록이므로 이전 배포 대비 변경사항은 없습니다.

| 파일 | 버전 | 모듈 설명 |
|---|---|---|
| shared/include/common.h | V1 | common.h: 다른 모듈이 사용하는 자료형·함수 선언입니다. 구현과 자세한 학습 설명은 .c에 있습니다. |
| shared/include/company.h | V1 | company.h: 다른 모듈이 사용하는 자료형·함수 선언입니다. 구현과 자세한 학습 설명은 .c에 있습니다. |
| shared/include/gameConfig.h | V1 | gameConfig.h: 다른 모듈이 사용하는 자료형·함수 선언입니다. 구현과 자세한 학습 설명은 .c에 있습니다. |
| shared/include/gameControl.h | V1 | gameControl.h: 다른 모듈이 사용하는 자료형·함수 선언입니다. 구현과 자세한 학습 설명은 .c에 있습니다. |
| shared/include/gameData.h | V1 | gameData.h: 다른 모듈이 사용하는 자료형·함수 선언입니다. 구현과 자세한 학습 설명은 .c에 있습니다. |
| shared/include/history.h | V1 | history.h: 다른 모듈이 사용하는 자료형·함수 선언입니다. 구현과 자세한 학습 설명은 .c에 있습니다. |
| shared/include/input.h | V1 | input.h: 다른 모듈이 사용하는 자료형·함수 선언입니다. 구현과 자세한 학습 설명은 .c에 있습니다. |
| shared/include/listing.h | V1 | listing.h: 다른 모듈이 사용하는 자료형·함수 선언입니다. 구현과 자세한 학습 설명은 .c에 있습니다. |
| shared/include/market.h | V1 | market.h: 공포지수와 시장 상태. 100bp = 1%입니다. |
| shared/include/order.h | V1 | order.h: 다른 모듈이 사용하는 자료형·함수 선언입니다. 구현과 자세한 학습 설명은 .c에 있습니다. |
| shared/include/portfolio.h | V1 | portfolio.h: 다른 모듈이 사용하는 자료형·함수 선언입니다. 구현과 자세한 학습 설명은 .c에 있습니다. |
| shared/include/random.h | V1 | random.h: 다른 모듈이 사용하는 자료형·함수 선언입니다. 구현과 자세한 학습 설명은 .c에 있습니다. |
| shared/include/saveLoad.h | V1 | saveLoad.h: 다른 모듈이 사용하는 자료형·함수 선언입니다. 구현과 자세한 학습 설명은 .c에 있습니다. |
| shared/include/screenOut.h | V1 | screenOut.h: 다른 모듈이 사용하는 자료형·함수 선언입니다. 구현과 자세한 학습 설명은 .c에 있습니다. |
| shared/include/session.h | V1 | session.h: main이 사용하는 실행 상태와 보조 함수입니다. |
| shared/include/stockPrice.h | V1 | stockPrice.h: 다른 모듈이 사용하는 자료형·함수 선언입니다. 구현과 자세한 학습 설명은 .c에 있습니다. |
| shared/include/stockQuery.h | V1 | stockQuery.h: 다른 모듈이 사용하는 자료형·함수 선언입니다. 구현과 자세한 학습 설명은 .c에 있습니다. |
| shared/include/trade.h | V1 | trade.h: 다른 모듈이 사용하는 자료형·함수 선언입니다. 구현과 자세한 학습 설명은 .c에 있습니다. |
| shared/src/company.c | V1 | 회사 고유 ID, 이름, 업종, 재무 정보와 최초 종목 구성을 관리합니다. 생성·갱신 책임을 이 파일에 모았습니다. |
| shared/src/gameControl.c | V1 | 기존 시장·재무·가격·폐지·체결 순서를 재사용하여 안전하게 한 번에 반영합니다. |
| shared/src/history.c | V1 | 배열이 가득 차면 오래된 자료를 하나 제거하고 새 자료를 끝에 넣습니다. 연결 리스트나 트리를 사용하지 않습니다. |
| shared/src/input.c | V1 | scanf가 Enter를 기다리며 실행을 멈추는 방식 대신 문자 단위로 확인합니다. 입력 중에도 main의 반복문이 계속 실행됩니다. |
| shared/src/listing.c | V1 | 가격 계산과 거래 체결 사이에 폐지 대상을 먼저 처리합니다. 사용자에게 1,000원 이하 주식을 매도할 틈을 주지 않습니다. |
| shared/src/market.c | V1 | market.c: 시장 전체의 심리를 관리합니다. |
| shared/src/order.c | V1 | 미체결 배열을 접수 순서로 관리하고 중복 사용을 막습니다. 체결 여부를 결정한 뒤 실제 자산 변경은 trade에 맡깁니다. |
| shared/src/portfolio.c | V1 | 실제 현금과 예약 현금을 구분하고 수수료·비례 원가를 계산합니다. 화면도 같은 계산을 사용합니다. |
| shared/src/random.c | V1 | 가격 변화와 신규 회사, 공포지수의 무작위 선택을 한 난수 상태로 통합합니다. rand/srand를 다시 호출하지 않습니다. |
| shared/src/saveLoad.c | V1 | 구조체 전체를 fwrite하지 않고 모든 필드를 명시적으로 기록합니다. 복구한 데이터도 거래 예약 규칙에 맞는지 검사합니다. |
| shared/src/screenOut.c | V1 | 윈도 콘솔의 좌표에 쓰며 줄바꿈으로 화면을 쌓지 않습니다. 목록 10개, 거래 내역 5건 단위로 페이지를 나눕니다. |
| shared/src/session.c | V1 | 저장 복원·명령 처리·시간 계산을 돕습니다. 프로그램의 반복문과 입력 흐름은 main.c에 있습니다. |
| shared/src/stockPrice.c | V1 | stockPrice.c: 공통 심리 + 업종 + 기업 재무 + 개별 난수로 등락률을 만듭니다. |
| shared/src/stockQuery.c | V1 | 30개 원본 회사를 재배치하지 않고 위치 배열을 정렬합니다. 그래야 보유 수량이 다른 회사와 섞이지 않습니다. |
| shared/src/trade.c | V1 | 거래가 성립한 한 순간의 자산 변경을 담당합니다. 매수 원가에는 매수 수수료를 포함합니다. |
| production/main.c | V1 | main.c: 프로그램의 실행 순서를 여기서 직접 읽습니다. |
| presentation/main.c | V1 | main.c: 프로그램의 실행 순서를 여기서 직접 읽습니다. |
| presentation/presentation.c | V1 | 진행 속도와 사건 재현은 이 프로젝트가 소유합니다. |
| presentation/presentation.h | V1 | #ifndef PRESENTATION_H |
| tests/test_game.c | V1 | 기존 V2 테스트를 재사용하고 V3 저장·파일 개수 검증을 추가했습니다. |
