def test_screen_two():
    for num in range(0, 100):
        col = int(num * 0.01 * 2.0)
        y = (num * 0.01 - float(col) / 2.0) * 2.0
        newY = y / 960.0 * 480.0 + 1.0 / 4.0
        print('col:' + str(col) + ' num: ' + str(num) + ' y: ' + str(y) + ' newY: ' + str(newY))
        # print(str(newY))

if __name__ == "__main__":
    test_screen_two()