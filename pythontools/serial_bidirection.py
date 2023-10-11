import serial
import argparse
import threading
import time
import signal


ARGS = None             # args object
SER = None              # serial object
LISTEN_THREAD = None    # serial listening thread
RUNNING = True          # running flag


# argparser, get serial port file path from command line (unix systems only)
def argparser():
    parser = argparse.ArgumentParser()
    parser.add_argument('-s', '--serial', type=str, nargs=1, help='serial file path')
    global ARGS
    ARGS = parser.parse_args()


# function for serial listening thread.
def serial_listen():
    global SER
    while True:
        try:
            serial_input = SER.readline().decode("utf-8")
            print(serial_input)
        except Exception as e:
            if RUNNING:
                print(e)
            break


# handle keyboard interrupt
def keyboard_interrupt_handler(signal, frame):
    global SER 
    global LISTEN_THREAD
    global RUNNING
    RUNNING = False
    SER.close()
    LISTEN_THREAD.join()
    exit(0)


def main():
    global SER
    global LISTEN_THREAD

    signal.signal(signal.SIGINT, keyboard_interrupt_handler)

    argparser()
    SER = serial.Serial(ARGS.serial[0], 115200)
    
    LISTEN_THREAD = threading.Thread(target=serial_listen)
    LISTEN_THREAD.start()
    
    while True:
        message = input("> ")
        message = message.encode("utf-8") + b"\r"
        try:
            SER.write(message)
        except Exception as e:
            print(e)
            break
        time.sleep(0.1)             # prevent input and output mess up
    

if __name__ == "__main__":
    main()
