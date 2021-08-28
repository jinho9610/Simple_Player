# Simple_Player
+ openGL(glfw) 기반으로 동영상(only Video) 파일을 재생하는 간단한 플레이어
+ 동영상 한 프레임의 RAW Data는 openCV를 이용하여 가져옴
+ Data Feeding 부분과 Rendering 부분을 구분하기 위해 Thread 이용 및 동기화 진행(unique_lock, condition_variable, etc.)
