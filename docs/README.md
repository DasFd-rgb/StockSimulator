# StockGame V2

압축 해제 후 일반용은 production/run.cmd, 시연용은 presentation/run.cmd로 실행한다. Visual Studio에서는 StockSimulatorV2.sln을 연다.

일반용에서 restart를 입력하고 restart yes로 확인하면 새 게임을 시작한다. restart no로 취소할 수 있다. 성공 직후에는 새 게임을 복구 파일에 기록하고 q로 나갈 때 본 저장을 갱신한다.

기존 저장을 가져오려면 해당 프로그램의 data 폴더를 V2의 같은 프로그램 폴더에 복사한다. 일반용과 시연용은 별도 폴더다. 파일명 game_v4.sav는 바이너리 호환을 위해 유지하며, 화면과 배포 버전은 V2이다. 배포 압축에는 플레이 데이터가 들어 있지 않다.

StockGame_V2_Change_Report.pdf에서 파일별 버전과 변경 사항을 확인할 수 있다. AI용 원문은 StockGame_V2_Changes_AI.md이다.
