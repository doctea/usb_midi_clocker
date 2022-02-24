

mults = [ 4, 2, 1, 0.5 ]

for x in mults:
    for i in range(0,8):
        if (i % x == 0):
            print("x ", end="")
        else:
            print("_ ", end="")
    print()

print()
