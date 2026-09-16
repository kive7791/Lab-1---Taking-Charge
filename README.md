# Lab-1---Taking-Charge
A project that involved building a small ESP32-based device using a starter kit to detect touch inputs, recognize simple swiping gestures, and provide audio feedback through a buzzer.

## Demo Video link
Youtube link: [Lab 1 video](https://youtu.be/LmTpR_5ryJ4)

If the link is not working, please go to the folder titled "Video" and watch it there.

## What is the interactive system
The interactive system uses 3 capacitive touch sensors connected to the ESP32 to detect indicual toches and left or right swiping guestures, with a buzzer attached to provide audio feedback.

## How it runs
To run the system:
1. Connect ESP32 to computer
2. upload the code from "Lab_1.ino"
3. Follow the serial monitor prompts to calibrate each touch sensor (32,27,13)
4. Touch and swipe to interact with the device.
    - Audio detection when:
        1. completing a swip guesture
        2. when touching only pin 27 (middle) 

## Extra things used in order to complete this lab

### Youtube video use
I watched a short YouTube video in order to connect a buzzer to my device, and to understand basic buzzer functions.

Youtube link: https://youtu.be/otoux_SXL8E?si=EsG8sA0C5XrQJ9e8
### AI use disclosure

  Explaination: I was having an issue where I believed my board had an old version of code on it; therefore I used AI as a check/to ask how I can re-do this. 

  Input: Old code is still uploaded to my chip, esp-32. How can I refresh the board?
  My serial monitor upload
  and my current code. 

  Summary: Basically, I rebooted my system. Took the AI's suggestions. It found an error, which I had figured was a problem but now I knew why my function wasn't running.
  
  AI Output:
  1. Your ESP32 is not necessarily still running the old sketch
        This output:

        rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
        ...
        Starting calibration.
        Get ready to calibrate SENSOR 32.
        Touch AND release Sensor 32 SEVERAL times.
        Calibration starting...
        Calibrating sensor 32...

        is clearly from your older program.

        But later you get:

        Untouched baseline: 1574
        Touch the sensor to begin calibration...

        That text is from your new program.

        So I suspect you're seeing a mixture of:

        old Serial Monitor output that was already displayed,
        ESP32 boot messages,
        output from the newly uploaded program,
        and resets/reboots.

        The important line is this:

        boot:0x3 (DOWNLOAD_BOOT(UART0/UART1/SDIO_REI_REO_V2))
        waiting for download

        That means the ESP32 entered its firmware download/bootloader mode. It isn't running your application at that moment.

    2. How to cleanly refresh the ESP32

        You do not normally need to erase the ESP32 before uploading a new sketch. Uploading replaces the existing program.

        I recommend this sequence:

        Disconnect the ESP32 USB cable.
        Close the Serial Monitor.
        Reconnect USB.
        Verify Tools → Board is your correct ESP32 board.
        Verify Tools → Port is the ESP32's current COM port.
        Compile/upload your current sketch.
        Wait until Arduino reports the upload is complete.
        Press the ESP32's EN/RESET button once.
        Open Serial Monitor at 115200 baud.

        You should then see your current code begin with:

        Starting calibration.

        Calibrating sensor 32...
        DO NOT TOUCH THE SENSOR...
        Measuring untouched baseline.

        You should not see:

        Get ready to calibrate SENSOR 32.
        Touch AND release Sensor 32 SEVERAL times.
        Calibration starting...

        because those strings aren't present in the current code you posted.

        If it says waiting for download

        If you see:

        boot:0x3 (DOWNLOAD_BOOT...)
        waiting for download

        press the EN/RESET button without holding BOOT.

        The BOOT button forces the ESP32 into the download mode you're seeing.

    3. But your current calibration code has a bug

        Even after we resolve the upload/reset issue, your new calibration will hang at:

        Touch the sensor to begin calibration...

        I see the problem in your code.

        You currently do this:

        sensorRange calibration(int touchPin, long caliTime) {
        sensorRange range;

        int sensedValue = touchRead(touchPin);

        That reads the sensor once.

        Later:

        while (true) {
        if (sensedValue < (baseline - START_TOUCH_OFFSET)){
            Serial.println("Touch detected!");
            break;
        }
        delay(10);
        }

        But sensedValue never changes inside that loop.

        So imagine:

        Initial sensedValue = 1570
        Baseline           = 1574
        Touch threshold    = 1474

        The ESP32 repeatedly asks:

        1570 < 1474

        which is always false.

        You could touch the sensor for an hour and the program would never know.

        Fix #1 — read the sensor inside the loop

        Change this:

        while (true) {
        if (sensedValue < (baseline - START_TOUCH_OFFSET)){
            Serial.println("Touch detected!");
            break;
        }
        delay(10);
        }

        to:

        while (true) {

        sensedValue = touchRead(touchPin);

        if (sensedValue < (baseline - START_TOUCH_OFFSET)) {
            Serial.println("Touch detected!");
            break;
        }

        delay(10);
        }
    4. You have the same bug during min/max calibration

        Your active calibration currently says:

        while ((millis() - startTime) < caliTime) {

        if (sensedValue < range.min) {
            range.min = sensedValue;
        }

        if (sensedValue > range.max) {
            range.max = sensedValue;
        }

        delay(10);
        }

        Again, you're never calling:

        touchRead(touchPin)

        inside the loop.

        Therefore your min and max will effectively become the same value.

        Fix it:

        while ((millis() - startTime) < caliTime) {

        sensedValue = touchRead(touchPin);

        if (sensedValue < range.min) {
            range.min = sensedValue;
        }

        if (sensedValue > range.max) {
            range.max = sensedValue;
        }

        delay(10);
        }
    5. Corrected calibration() function

        You don't need to rewrite the entire program. Replace your current calibration() with this:

        sensorRange calibration(int touchPin, unsigned long caliTime) {

        sensorRange range;


        // ==========================================================
        // STEP 1 - MEASURE UNTOUCHED BASELINE
        // ==========================================================

        Serial.println("DO NOT TOUCH THE SENSOR...");
        Serial.println("Measuring untouched baseline.");

        delay(1000);

        int baseline = getBaseline(touchPin, BASELINE_TIME);

        Serial.print("Untouched baseline: ");
        Serial.println(baseline);


        // ==========================================================
        // STEP 2 - WAIT UNTIL USER TOUCHES SENSOR
        // ==========================================================

        Serial.println("Touch the sensor to begin calibration...");

        int sensedValue;

        while (true) {

            // IMPORTANT:
            // Continuously get a NEW sensor reading.
            sensedValue = touchRead(touchPin);

            if (sensedValue < (baseline - START_TOUCH_OFFSET)) {

            Serial.println("Touch detected!");
            break;
            }

            delay(10);
        }


        // ==========================================================
        // STEP 3 - ACTIVE CALIBRATION
        // ==========================================================

        Serial.println();
        Serial.println("Calibration started!");
        Serial.println("Touch and release the sensor SEVERAL times.");

        range.min = 10000;
        range.max = 0;

        unsigned long startTime = millis();


        // ==========================================================
        // STEP 4 - COLLECT MIN AND MAX VALUES
        // ==========================================================

        while ((millis() - startTime) < caliTime) {

            // IMPORTANT:
            // Get a NEW sensor reading every time through the loop.
            sensedValue = touchRead(touchPin);

            if (sensedValue < range.min) {
            range.min = sensedValue;
            }

            if (sensedValue > range.max) {
            range.max = sensedValue;
            }

            delay(10);
        }


        // ==========================================================
        // STEP 5 - SHOW RESULTS
        // ==========================================================

        Serial.println("Calibration period complete!");

        Serial.print("Minimum (touched): ");
        Serial.println(range.min);

        Serial.print("Maximum (untouched): ");
        Serial.println(range.max);

        return range;
        }

    6. Also fix your Serial output order

        You currently have:

        Serial.println("Untouched baseline: ");
        Serial.print(baseline);

        which produces awkward output such as:

        Untouched baseline:
        1574Touch the sensor to begin calibration...

        Reverse the print/println:

        Serial.print("Untouched baseline: ");
        Serial.println(baseline);

        Then you get:

        Untouched baseline: 1574
        Touch the sensor to begin calibration...

        Much easier to debug.

    7. I'd shorten BASELINE_TIME

        You currently have:

        const int BASELINE_TIME = 10000;

        That's 10 seconds just to measure the untouched baseline, followed by another 10 seconds of active calibration, for every sensor.

        For three sensors that's at least:

        3 × (10 sec baseline + waiting + 10 sec calibration)

        = at least 60 seconds

        You don't need 10 seconds to establish an untouched average for this lab. I'd use:

        const unsigned long BASELINE_TIME = 2000;

        and retain:

        const unsigned long CALIBRATION_TIME = 10000;

        That gives you roughly 200 untouched measurements because you're sampling every 10 ms. That's plenty for establishing a simple average.

    8. Expected behavior after uploading

        Once you upload the corrected program and press EN/RESET, leave sensor 32 alone:

        Starting calibration.

        Calibrating sensor 32...
        DO NOT TOUCH THE SENSOR...
        Measuring untouched baseline.

        Untouched baseline: 1574
        Touch the sensor to begin calibration...

        It should stop there.

        Touch sensor 32:

        Touch detected!

        Calibration started!
        Touch and release the sensor SEVERAL times.

        Now repeatedly touch/release 32 for ten seconds.

        Then you should get something resembling:

        Calibration period complete!
        Minimum (touched): 425
        Maximum (untouched): 1581


        Calibrating sensor 27...
        DO NOT TOUCH THE SENSOR...
        Measuring untouched baseline.

        Untouched baseline: 1415
        Touch the sensor to begin calibration...

        Then repeat for 27 and 13.

        One more thing I want to check after this

        Your console showed:

        Untouched baseline: 1574
        ...
        Untouched baseline: 1570

        Those values look very reasonable and stable, so the baseline calculation appears to be working. The failure to recognize your touch is explained by the stale sensedValue variable.

        Fix those two locations where touchRead() needs to happen repeatedly, upload again, press EN/RESET, and your calibration architecture should behave the way you originally intended.

        Also, the ESP32 boot text such as:

        rst:0x1 (POWERON_RESET)
        mode:DIO
        load:...
        entry...

        is normal after reset. The DOWNLOAD_BOOT ... waiting for download message is the one indicating that it booted into programming mode rather than your sketch.
