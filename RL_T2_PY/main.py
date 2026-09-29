from multiprocessing import shared_memory

import struct
import time
import random

import torch

SHARED_MEMORY_NAME = "Global\\Chaser_Memory_1"
SHARED_MEMORY_SIZE = 64

shared = None

def print_table(table : torch) :
    print(f"---")
    for row_idx in range(3) :
        raw = table[row_idx]
        print(f"{raw[0].item()}   {raw[1].item()}   {raw[2].item()}")
    print(f"---")


class SharedMemoryType() :

    def __init__(self) :

        self.shared = shared_memory.SharedMemory(
            name = SHARED_MEMORY_NAME,
            create = True,
            size = SHARED_MEMORY_SIZE
        )

    def get_currentState(self) -> int :
        # 튜플을 반환한다
        current_state, =  struct.unpack("<i", self.shared.buf[0:4])
        return current_state

    def get_nextState(self) -> int :
        next_state, = struct.unpack("<i", self.shared.buf[4:8])
        return next_state

    def set_action(self, newAction : int) :
        self.shared.buf[8:12] = struct.pack('<i', newAction);
        return

    def get_reward(self) -> float :
        reward, = struct.unpack["<f", self.shared.buf[12:16]] 
        return reward

    def get_stateSetted(self) -> int :
        state_setted, = struct.unpack("<i", self.shared.buf[16:20])
        return state_setted

    def set_stateSetted(self, newTargetDir : int) :
        self.shared.buf[16:20] = struct.pack("<i", 0);

    def set_actionSetted(self, newAction : int) :
        self.shared.buf[20:24] = struct.pack("<i", newAction)

    def set_rewardSetted(self, newRewardSetted : int):
        self.shared.buf[24:28] = struct.pack("<i", newRewardSetted)

    def get_rewardSetted(self) :
        reward_setted, = struct.unpack("<i", self.shared.buf[24:28])
        return reward_setted

    def get_training(self) :
        training, = struct.unpack("<i", self.shared.buf[28:32])
        return training

    def close(self) :

        if self.shared != None :
            # 현재 Python 에서 연결 해제
            self.shared.close()
            # 운영체제에서 공유 메모리 이름 제거
            self.shared.unlink()

def main():

    print("Hello from rl-t2-py!")

    shared_memory = SharedMemoryType()

    q_table = torch.zeros((3,3))

    while True :

        if shared_memory.get_training() == 1 :
            print("training 시작 대기중...")
            time.sleep(0.5)
            break;

        # state 확인
        if shared_memory.get_stateSetted() != 1 :
            print("상태가 설정될때까지 대기중...")
            time.sleep(0.5)
            continue
        
        current_state = shared_memory.get_currentState()
        shared_memory.set_stateSetted(0) # 가져갔다고 표시

        # 판단
        selected_action = -1

        if random.random() <= 0.2 :

            # 랜덤한 판단
            selected_action = random.randint(0, 2)
        else : 
            # table 에서 가장 가능성 높은거 
            selected_action = q_table[current_state].argmax().item()

        # 공유 메모리 적재
        shared_memory.set_action(selected_action)
        shared_memory.set_actionSetted(1)

        # 보상 기다림
        reward = 0
        next_state = -1
        while True : 
            
            if 1 == shared_memory.get_rewardSetted() :
                reward = shared_memory.get_reward()
                shared_memory.set_rewardSetted(0)
                next_state = shared_memory.get_nextState()
                
                break

            time.sleep(0.2)

        gamma = 0.2
        # q-learning
        # 1-gamma 부분은 현재의 보상이고,
        # gamma 부분은, 미래에 내가 할 선택에 있을지도 모르는 가치이다.
        # 내가 현재는 행동해서 (1-gamma)*reward 만큼 보상을 받았지만,
        # 다음 상태를 보아하니, 다음의 선택에서 잘하면, 엄청난 보상이 있겠구나.
        # 그 기대만큼을 반영해서, 현재의 선택은 얼마만큼의 가치가 있었다, 
        # 내 지금행동으로 인해서 상태가 바뀌었는데, 그 바뀐 상태가 좋으면 좋을수록,
        # 내가 미래에 얻게될 보상도 좋을것이기 때문에, 현재의 선택도 그만큼 가치가 있었다. 라는것을 반영
        target = (1-gamma)*reward + (gamma)*q_table[next_state].max()

        # 때문에 이번 선택을 얼마만큼의 가치가 있었는데, 학습률에 비례해서 더할꺼야.
        learning_rate = 0.2
        q_table[current_state, selected_action] += learning_rate * (target - q_table[current_state, selected_action])
        print(f"{current_state} : {selected_action} = {reward} ")
        print_table(q_table)
        
    # 결과 학인
    print("최종결과")
    print_table(q_table)

if __name__ == "__main__":
    main()
