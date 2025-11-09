from serial import Serial, SerialException
from time import sleep


def main():
    with Serial("/dev/ttyACM0", 9600, timeout=1) as ser:
        for i in range(5):
            try:
                ser.write(b"\xff")
                sleep(1)
                ser.write(b"\x77")
                sleep(1)
            except SerialException:
                print("Heh")
                continue

        ser.write(b"\x69")


if __name__ == "__main__":
    main()
