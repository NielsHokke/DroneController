try:
    from msvcrt import getch
    print("Windows machine")
except ImportError:
    import getch
    print("Linux machine")


if __name__ == '__main__':
    while True:
        print("hoooi {}".format(getch()))
