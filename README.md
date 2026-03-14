# 🌱 봄날의 햇살

2026 디콘 캡스톤 디자인 프로젝트

손의 움직임과 LLM을 통해 결정된 계절·환경 컨셉을 기반으로,  
빛에 반응하는 환경과 식물의 성장을 경험하는  
전시형 인터랙티브 디스플레이 프로젝트

# 👥 팀원 소개

* 최재원(https://github.com/choi06151)

시각적 효과 및 연출 전문

* 정수연(https://github.com/suyeon09)

해프닝

* 김동현(https://github.com/Baek1213)

스피드 트리 연결 전문

* 김수진(https://github.com/kdjh2002)

립모션 연결 전문



최초에 해야할 일 

1. Google Drive Desktop 설치

자신의 PC에 로컬디스크 : C 말고도 드라이브 전용 G 폴더가 생성됌


2.웹 드라이브에서 공유 받고 있는 Capstone-Sunlight/DriveAssets 을 내 드라이브에 추가 



3. Capstone-Sunlight/DriveAssets 폴더 오프라인 사용 가능 설정

해주지 않으면 경로가 깨집니다
(드라이브파일에 들어가서 우클릭후 추가 옵션 보기 하면 나옵니다)

4. Git 프로젝트 clone
이후 클론 및 실행은 정상적으로 가능하지만 현재 DriveAssets 폴더가 비어 있어서 정상적으로 작동하지 않습니다


5. Content 폴더에서 아래 명령 실행

mklink /J DriveAssets "G:\내 드라이브\Capstone-Sunlight\DriveAssets"

DriveAssets 다음 내용은 자신의 드라이브 전용 폴더에서 Capstone-Sunlight/DriveAssets의 경로 복사해서 붙여넣기 하면됩니다

이후 부턴 정상 작동 가능합니다


**6..깃 Bash에 명령어 입력하기**

Git Bash에 명령에 입력해주시면 추후 충돌 확률 적어집니다

---

cd "해당 프로젝트파일 경로 입력"

Install Git LFS

git lfs install

---
--->이거는 데스크탑에서 클론받을때 그냥 하라하니까 하면 됍니다


권장 사항입니다 똑같이 입력해주세요

---

git config --global core.autocrlf false

---


**추후 프로젝트 사용법**

Content폴더가 현재  DriveAssets,GitAssets으로 세분화 되어 있습니다

말그대로 

DriveAssets은 구글드라이브 애셋을 저장하는 장소

GitAssets은 GIt을 이용해 다루는 것들을 다룹니다



추후 프로젝트 하실때 GitAssets내부에
자신의 이니셜을 딴 EX)CJW 폴더를 만들고 그안에서 작업 해 주세요


무거운 파일(Ex: fab에서 다운받는 것들은 DriveAssets에 저장)
가벼운 파일 (Ex:우리가 작업을 위해 만드는 블루프린트나 혹은 등등 은 GitAssets에 저장) 


브랜치도 Main 브랜치를 제외한 자신만의 서브 브랜치에서 푸시하고

리베이스 하는 방식으로 프로젝트 폭파를 방지해야합니다 

방법이 복잡해서 이건 다음에 ,,










