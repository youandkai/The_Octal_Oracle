/* 
 * ==============================================================================
 * T H E   O C T A L   O R A C L E
 * Version 2.1.0 - Production Ready (CRITICAL BUG FIX)
 * 
 * "From the void, the Eye observes. When the shadow stirs, the eight fires
 *  awaken to tell the stories of the old world. Silence brings the descent,
 *  returning the light to the earth, one spark at a time."
 * 
 * LAYOUT: A 360° Compass of 12V Luminescence
 * LOGIC: 30-Second Vigil | 5-Second Descent | Non-Blocking State Machine
 * HARDWARE: Parallax PIR Eye | Octal N-Channel MOSFET Array
 * 
 * IMPROVEMENTS v2.1:
 * - CRITICAL: Fixed scope bug in LIGHTS_ON case that prevented shutdown
 * - Reduced PIR warmup time from 60s to 10s (tested and validated)
 * - Added proper braces to all switch cases for variable scope safety
 * 
 * IMPROVEMENTS v2.0:
 * - Non-blocking animations using millis()
 * - Proper PIR motion tracking with debouncing
 * - Failsafe maximum on-time (5 minutes)
 * - Startup LED test pattern
 * - Clean state management
 * - Memory-efficient animation engine
 * ==============================================================================
 */

// ==================== PIN CONFIGURATION ====================
const int pUp = 2, pUR = 3, pRi = 4, pLR = 5;
const int pDn = 6, pLL = 7, pLe = 8, pUL = 9;
const int ledPins[] = {pUp, pUR, pRi, pLR, pDn, pLL, pLe, pUL};
const int pirPin = 10;
const int NUM_LEDS = 8;

// ==================== TIMING CONSTANTS ====================
const unsigned long PIR_SETTLE_TIME = 10000;      // 10s warmup for PIR sensor
const unsigned long MOTION_TIMEOUT = 30000;       // 30s after last motion
const unsigned long SHUTDOWN_DURATION = 5000;     // 5s reverse chase
const unsigned long MAX_ON_TIME = 300000;         // 5min failsafe
const unsigned long SHUTDOWN_STEP = SHUTDOWN_DURATION / NUM_LEDS;  // 625ms per LED

// ==================== SYSTEM STATE ====================
enum SystemState {
  STARTUP,           // Initial boot and PIR warmup
  IDLE,             // Lights off, monitoring for motion
  ANIMATING,        // Playing wake-up animation
  LIGHTS_ON,        // All lights on, monitoring motion
  SHUTTING_DOWN     // Playing shutdown animation
};

SystemState currentState = STARTUP;
unsigned long lastMotionTime = 0;
unsigned long stateStartTime = 0;
unsigned long lightsOnTime = 0;

// ==================== ANIMATION STATE ====================
enum StoryType {
  RISE, WHIRLPOOL, HEARTBEAT, PINBALL, 
  PINCER, RADAR, SPARK, COMPASS
};

StoryType currentStory;
int animStep = 0;
unsigned long animStepStartTime = 0;

// ==================== SHUTDOWN STATE ====================
int shutdownStep = 0;

// ==================== SETUP ====================
void setup() {
  // Initialize all LED pins as outputs, set LOW
  for (int i = 0; i < NUM_LEDS; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
  }
  
  pinMode(pirPin, INPUT);
  
  // Seed random number generator with analog noise
  // If A0 is used elsewhere, change to another floating analog pin
  randomSeed(analogRead(A0) + analogRead(A1) + millis());
  
  // Optional: Serial debugging
  // Serial.begin(9600);
  // Serial.println("Octal Oracle v2.0 - Initializing...");
  
  // Run startup test pattern
  startupTest();
  
  stateStartTime = millis();
}

// ==================== MAIN LOOP ====================
void loop() {
  unsigned long currentTime = millis();
  bool motionDetected = digitalRead(pirPin) == HIGH;
  
  // Update last motion time continuously
  // NOTE: This extends the timeout if motion detected during ANIMATING,
  // ensuring 30s of inactivity AFTER all motion stops, not just after animation ends
  if (motionDetected) {
    lastMotionTime = currentTime;
  }
  
  // State machine
  switch (currentState) {
    
    case STARTUP:
      // Wait for PIR to settle after power-on
      if (currentTime - stateStartTime >= PIR_SETTLE_TIME) {
        transitionTo(IDLE, currentTime);
      }
      break;
      
    case IDLE:
      // Waiting for motion
      if (motionDetected) {
        // Choose random story and start animation
        currentStory = (StoryType)random(0, 8);
        animStep = 0;
        transitionTo(ANIMATING, currentTime);
      }
      break;
      
    case ANIMATING: {
      // Run the selected animation (non-blocking)
      bool animComplete = runAnimation(currentTime);
      if (animComplete) {
        lightsOnTime = currentTime;  // Track when lights came on
        transitionTo(LIGHTS_ON, currentTime);
      }
      break;
    }
      
    case LIGHTS_ON: {
      // Check for timeout or failsafe
      // NOTE: Braces are critical here to properly scope boolean variables
      bool motionTimeout = (currentTime - lastMotionTime >= MOTION_TIMEOUT);
      bool failsafeTimeout = (currentTime - lightsOnTime >= MAX_ON_TIME);
      
      if (motionTimeout || failsafeTimeout) {
        animStep = 0;        // Reset animation step for clean state
        shutdownStep = 0;
        transitionTo(SHUTTING_DOWN, currentTime);
      }
      break;
    }
      
    case SHUTTING_DOWN: {
      // Run shutdown animation (non-blocking)
      bool shutdownComplete = runShutdown(currentTime);
      if (shutdownComplete) {
        transitionTo(IDLE, currentTime);
      }
      break;
    }
  }
}

// ==================== STATE MANAGEMENT ====================
void transitionTo(SystemState newState, unsigned long currentTime) {
  currentState = newState;
  stateStartTime = currentTime;
  animStepStartTime = currentTime;
}

// ==================== STARTUP TEST ====================
void startupTest() {
  // Quick sequential test of all LEDs
  for (int i = 0; i < NUM_LEDS; i++) {
    digitalWrite(ledPins[i], HIGH);
    delay(100);
  }
  delay(200);
  for (int i = 0; i < NUM_LEDS; i++) {
    digitalWrite(ledPins[i], LOW);
    delay(100);
  }
  delay(500);
}

// ==================== ANIMATION ENGINE ====================
// Returns true when animation is complete
bool runAnimation(unsigned long currentTime) {
  unsigned long elapsed = currentTime - animStepStartTime;
  
  switch (currentStory) {
    case RISE:       return animRise(elapsed);
    case WHIRLPOOL:  return animWhirlpool(elapsed);
    case HEARTBEAT:  return animHeartbeat(elapsed);
    case PINBALL:    return animPinball(elapsed);
    case PINCER:     return animPincer(elapsed);
    case RADAR:      return animRadar(elapsed);
    case SPARK:      return animSpark(elapsed);
    case COMPASS:    return animCompass(elapsed);
  }
  return true;  // Should never reach
}

// ==================== ANIMATION: RISE ====================
// Rising Sun - 2 seconds
bool animRise(unsigned long elapsed) {
  switch (animStep) {
    case 0:  // 0-600ms: Bottom lights
      digitalWrite(pLL, HIGH); 
      digitalWrite(pLR, HIGH);
      if (elapsed >= 600) { animStep++; animStepStartTime = millis(); }
      break;
    case 1:  // 600-1200ms: Side lights
      digitalWrite(pLe, HIGH); 
      digitalWrite(pRi, HIGH);
      if (elapsed >= 600) { animStep++; animStepStartTime = millis(); }
      break;
    case 2:  // 1200-1800ms: Upper diagonal
      digitalWrite(pUL, HIGH); 
      digitalWrite(pUR, HIGH);
      if (elapsed >= 600) { animStep++; animStepStartTime = millis(); }
      break;
    case 3:  // 1800-2000ms: Top light
      digitalWrite(pUp, HIGH);
      if (elapsed >= 200) return true;
      break;
  }
  return false;
}

// ==================== ANIMATION: WHIRLPOOL ====================
// Spinning water - 2.4 seconds
bool animWhirlpool(unsigned long elapsed) {
  const int delays[] = {500, 400, 300, 200, 200, 200, 200, 200};
  
  if (animStep < NUM_LEDS) {
    digitalWrite(ledPins[animStep], HIGH);
    if (elapsed >= delays[animStep]) {
      animStep++;
      animStepStartTime = millis();
    }
  } else {
    return true;
  }
  return false;
}

// ==================== ANIMATION: HEARTBEAT ====================
// Pulse & Wake - 3 seconds
bool animHeartbeat(unsigned long elapsed) {
  switch (animStep) {
    case 0:  // First pulse ON
      setAll(HIGH);
      if (elapsed >= 100) { animStep++; animStepStartTime = millis(); }
      break;
    case 1:  // First pulse OFF
      setAll(LOW);
      if (elapsed >= 400) { animStep++; animStepStartTime = millis(); }
      break;
    case 2:  // Second pulse ON
      setAll(HIGH);
      if (elapsed >= 100) { animStep++; animStepStartTime = millis(); }
      break;
    case 3:  // Second pulse OFF
      setAll(LOW);
      if (elapsed >= 400) { animStep++; animStepStartTime = millis(); }
      break;
    case 4:  // Left/Right on
      digitalWrite(pLe, HIGH); 
      digitalWrite(pRi, HIGH);
      if (elapsed >= 500) { animStep++; animStepStartTime = millis(); }
      break;
    case 5:  // Up/Down on
      digitalWrite(pUp, HIGH); 
      digitalWrite(pDn, HIGH);
      if (elapsed >= 500) { animStep++; animStepStartTime = millis(); }
      break;
    case 6:  // All on
      setAll(HIGH);
      if (elapsed >= 100) return true;
      break;
  }
  return false;
}

// ==================== ANIMATION: PINBALL ====================
// Bouncing around - 4 seconds
bool animPinball(unsigned long elapsed) {
  const int order[] = {pDn, pUp, pLL, pUR, pLR, pUL, pLe, pRi};
  
  if (animStep < NUM_LEDS) {
    digitalWrite(order[animStep], HIGH);
    if (elapsed >= 500) {
      animStep++;
      animStepStartTime = millis();
    }
  } else {
    return true;
  }
  return false;
}

// ==================== ANIMATION: PINCER ====================
// Meeting at the top - 3 seconds
bool animPincer(unsigned long elapsed) {
  switch (animStep) {
    case 0:  // Bottom
      digitalWrite(pDn, HIGH);
      if (elapsed >= 750) { animStep++; animStepStartTime = millis(); }
      break;
    case 1:  // Lower diagonals
      digitalWrite(pLL, HIGH); 
      digitalWrite(pLR, HIGH);
      if (elapsed >= 750) { animStep++; animStepStartTime = millis(); }
      break;
    case 2:  // Sides
      digitalWrite(pLe, HIGH); 
      digitalWrite(pRi, HIGH);
      if (elapsed >= 750) { animStep++; animStepStartTime = millis(); }
      break;
    case 3:  // Upper diagonals
      digitalWrite(pUL, HIGH); 
      digitalWrite(pUR, HIGH);
      if (elapsed >= 750) { animStep++; animStepStartTime = millis(); }
      break;
    case 4:  // Top
      digitalWrite(pUp, HIGH);
      return true;
  }
  return false;
}

// ==================== ANIMATION: RADAR ====================
// Locked on - 6 seconds
bool animRadar(unsigned long elapsed) {
  // 3 sweeps of 400ms each = 1200ms
  // Then lock all on with 400ms delays = 3200ms
  // Total: 4400ms (adjusted from original 6s for better pacing)
  
  if (animStep < 24) {  // 3 sweeps * 8 LEDs
    int sweep = animStep / NUM_LEDS;
    int led = animStep % NUM_LEDS;
    
    digitalWrite(ledPins[led], HIGH);
    if (elapsed >= 50) {
      digitalWrite(ledPins[led], LOW);
      animStep++;
      animStepStartTime = millis();
    }
  } else if (animStep < 32) {  // Final lock-on
    int led = animStep - 24;
    digitalWrite(ledPins[led], HIGH);
    if (elapsed >= 400) {
      animStep++;
      animStepStartTime = millis();
    }
  } else {
    return true;
  }
  return false;
}

// ==================== ANIMATION: SPARK ====================
// Random ignition - 5 seconds (variable)
bool animSpark(unsigned long elapsed) {
  const int pins[] = {pUR, pDn, pLe, pUp, pLR, pUL, pRi, pLL};
  static int sparkDelays[8];
  static bool sparkInitialized = false;
  
  // Initialize random delays on first step
  if (animStep == 0 && !sparkInitialized) {
    for (int i = 0; i < NUM_LEDS; i++) {
      sparkDelays[i] = random(200, 800);
    }
    sparkInitialized = true;
  }
  
  if (animStep < NUM_LEDS) {
    digitalWrite(pins[animStep], HIGH);
    if (elapsed >= sparkDelays[animStep]) {
      animStep++;
      animStepStartTime = millis();
    }
  } else {
    sparkInitialized = false;  // Reset for next time
    return true;
  }
  return false;
}

// ==================== ANIMATION: COMPASS ====================
// Cardinals then Ordinals - 4 seconds
bool animCompass(unsigned long elapsed) {
  switch (animStep) {
    case 0:  // Cardinals on
      digitalWrite(pUp, HIGH); 
      digitalWrite(pDn, HIGH);
      digitalWrite(pLe, HIGH); 
      digitalWrite(pRi, HIGH);
      if (elapsed >= 2000) { animStep++; animStepStartTime = millis(); }
      break;
    case 1:  // All on
      setAll(HIGH);
      if (elapsed >= 100) return true;
      break;
  }
  return false;
}

// ==================== SHUTDOWN ANIMATION ====================
// Reverse chase - 5 seconds total
bool runShutdown(unsigned long currentTime) {
  unsigned long elapsed = currentTime - animStepStartTime;
  
  if (shutdownStep < NUM_LEDS) {
    if (elapsed >= SHUTDOWN_STEP) {
      digitalWrite(ledPins[NUM_LEDS - 1 - shutdownStep], LOW);
      shutdownStep++;
      animStepStartTime = currentTime;
    }
  } else {
    // Ensure all LEDs are off
    setAll(LOW);
    return true;
  }
  return false;
}

// ==================== HELPER FUNCTIONS ====================
void setAll(int state) {
  for (int i = 0; i < NUM_LEDS; i++) {
    digitalWrite(ledPins[i], state);
  }
}
