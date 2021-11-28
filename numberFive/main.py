for i in range(1, 1001):
    sum=0
    for j in range(i+1):
        sum += 0.875**j*0.75**(i-j)
    sum += 0.875**i
    if sum < 1:
        print(f"hello world {sum} and i = {i}")
        break
# print("no!!!")