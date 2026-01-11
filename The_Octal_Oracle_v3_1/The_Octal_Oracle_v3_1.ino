/* 
 * ==============================================================================
 * T H E   O C T A L   O R A C L E
 * Version 3.1.0 - EXPANDED STORIES with Adaptive Occupancy Detection
 * 
 * "From the void, the Eye observes. When the shadow stirs, the fires
 *  awaken to tell the stories of the old world and the new. Each awakening
 *  brings a different tale: storms and tides, celestial dance and earthly fury,
 *  all written in light upon the compass of eternity."
 * 
 * LAYOUT: A 360° Compass of 12V Luminescence
 * LOGIC: Adaptive Timeout (15s / 30s / 2min / 12min) | 5-Second Descent | Non-Blocking State Machine
 * HARDWARE: Parallax PIR Eye | Octal N-Channel MOSFET Array
 * OPTIMIZED FOR: Dance classes with periods of stillness
 * 
 * IMPROVEMENTS v3.1 - ADAPTIVE OCCUPANCY SYSTEM:
 * - Rising-edge detection for accurate motion event counting
 * - Motion confidence score (0-100) that decays over time
 * - Dynamic shutdown timeouts based on activity patterns:
 *   * 15s for likely false triggers (low confidence)
 *   * 30s for medium confidence
 *   * 2 min for high confidence (active room)
 *   * 12 min when "class lock" is engaged
 * - CLASS LOCK: Automatically engages when 6+ motion events in 2 minutes
 *   (indicates class arrival/group activity)
 * - Class lock protects against shutdown during still periods (meditation, stretching)
 * - Smart failsafe: 15 min normal / 2 hours during class lock
 * - Optional DEBUG_MODE for monitoring occupancy detection
 * 
 * BEHAVIOR SUMMARY:
 * - Empty room: Quick person passes through → lights on briefly (15-30s) then off
 * - Class arrives: Multiple people entering → CLASS LOCK engages
 * - During class: Even if dancers are still, lights stay on (12 min silence required)
 * - After class: 12 minutes of no motion → class lock releases, normal timing resumes
 * 
 * IMPROVEMENTS v3.0:
 * - EXPANDED: 8 new animations with narratives (16 total stories)
 * - LIGHTNING: Jagged electric strike from sky to ground
 * - TIDE: Ocean waves rolling in and retreating  
 * - ECLIPSE: Shadow passes across light and returns
 * - FIREFLY: Random twinkling dance of scattered lights
 * - AVALANCHE: Cascading tumble from peak to valley
 * - PENDULUM: Swinging side to side with acceleration
 * - SERPENT: Slithering coil in both directions
 * - NOVA: Stellar explosion bursting outward
 * 
 * IMPROVEMENTS v2.2.1:
 * - CRITICAL: Fixed RISE animation - now includes bottom light (pDn)
 * - RISE now properly animates from bottom to top: Dn → LL/LR → Le/Ri → UL/UR → Up
 * 
 * IMPROVEMENTS v2.2:
 * - CRITICAL: Fixed animation variety by using counter + random
 * - Even if RNG is poor, counter ensures all animations cycle through
 * - Each trigger advances by 1-3 positions, guaranteeing variety
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
const unsigned long SHUTDOWN_DURATION = 5000;     // 5s reverse chase
const unsigned long SHUTDOWN_STEP = SHUTDOWN_DURATION / NUM_LEDS;  // 625ms per LED

// ==================== PIR / OCCUPANCY HEURISTICS ====================
// Edge detection + refresh
const unsigned long EDGE_DEBOUNCE_MS = 800;     // ignore chatter edges
const unsigned long HIGH_REFRESH_MS  = 2000;    // if PIR stays HIGH, refresh lastMotionTime every 2s

// Dynamic shutdown choices
const unsigned long OFF_SHORT   = 15000;        // "shorter" (15s) - likely false trigger
const unsigned long OFF_30S     = 30000;        // medium confidence (30s)
const unsigned long OFF_2MIN    = 120000;       // high confidence (2 min)

// Class-lock behavior: protects "still dancers"
const unsigned long CLASS_LOCK_QUIET_OFF = 12UL * 60UL * 1000UL;  // 12 min of silence -> off
const unsigned long CLASS_FAILSAFE_ON    = 2UL * 60UL * 60UL * 1000UL; // 2 hours max when classLock
const unsigned long MAX_ON_TIME = 15UL * 60UL * 1000UL;  // 15 min failsafe when not in class

// Lock detection: "arrival/group pattern"
const unsigned long LOCK_WINDOW_MS = 120000;  // 2 minutes
const int LOCK_EDGES_MIN = 6;

// Motion score (confidence)
int motionScore = 0;                 // 0..100
const int SCORE_MAX   = 100;
const int SCORE_HIT   = 18;          // +score per motion edge
const int SCORE_DECAY = 1;           // -score per second
const unsigned long DECAY_TICK_MS = 1000;

// Edge history (for lock detection)
const int MAX_EDGES = 32;
unsigned long edgeTimes[MAX_EDGES];
int edgeHead = 0;
int edgeCount = 0;

// PIR state tracking
bool lastPirLevel = false;
unsigned long lastEdgeTime = 0;
unsigned long lastHighRefreshTime = 0;

bool classLock = false;

// ==================== DEBUG CONFIGURATION ====================
// Set to true to enable serial debugging (helps diagnose PIR issues)
const bool DEBUG_MODE = false;  // Change to true to enable debug output

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
  // Original 8
  RISE, WHIRLPOOL, HEARTBEAT, PINBALL, 
  PINCER, RADAR, SPARK, COMPASS,
  // New 8
  LIGHTNING, TIDE, ECLIPSE, FIREFLY,
  AVALANCHE, PENDULUM, SERPENT, NOVA
};

StoryType currentStory;
int animStep = 0;
unsigned long animStepStartTime = 0;
int animationCounter = 0;  // Counter to ensure variety even with poor RNG

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
  
  // Initialize serial debugging if enabled
  if (DEBUG_MODE) {
    Serial.begin(9600);
    while (!Serial && millis() < 3000) { }  // Wait up to 3s for serial
    Serial.println(F("=== OCTAL ORACLE v3.1 DEBUG MODE ==="));
    Serial.println(F("16 Animations | PIR Motion Tracking"));
    Serial.println(F("Monitoring PIR state, timing, and transitions..."));
    Serial.println();
  }
  
  // Run startup test pattern
  startupTest();
  
  stateStartTime = millis();
}

// ==================== MAIN LOOP ====================
void loop() {
  unsigned long currentTime = millis();

  // Read PIR level
  bool pirLevel = (digitalRead(pirPin) == HIGH);

  // Rising edge = a "motion event"
  bool motionEdge = pirLevel && !lastPirLevel && (currentTime - lastEdgeTime > EDGE_DEBOUNCE_MS);

  // Update last motion time:
  // - on edge (true event)
  // - and also while PIR is held HIGH (prevents timeout during sustained HIGH)
  if (motionEdge) {
    lastEdgeTime = currentTime;
    lastHighRefreshTime = currentTime;
    lastMotionTime = currentTime;

    pushEdge(currentTime);

    motionScore += SCORE_HIT;
    if (motionScore > SCORE_MAX) motionScore = SCORE_MAX;

    // Enter class lock if we see "arrival/group" density
    if (!classLock) {
      int edges2min = countEdgesInWindow(currentTime, LOCK_WINDOW_MS);
      if (edges2min >= LOCK_EDGES_MIN) {
        classLock = true;
        if (DEBUG_MODE) {
          Serial.print(F("*** CLASS LOCK ENGAGED *** ("));
          Serial.print(edges2min);
          Serial.println(F(" edges in 2 min)"));
        }
      }
    }
  }

  if (pirLevel && (currentTime - lastHighRefreshTime > HIGH_REFRESH_MS)) {
    lastHighRefreshTime = currentTime;
    lastMotionTime = currentTime;
  }

  // Score decay
  static unsigned long lastDecay = 0;
  if (currentTime - lastDecay >= DECAY_TICK_MS) {
    lastDecay = currentTime;
    if (motionScore > 0) motionScore -= SCORE_DECAY;
    if (motionScore < 0) motionScore = 0;
  }

  // Clear class lock only after a long quiet period
  if (classLock && (currentTime - lastMotionTime > CLASS_LOCK_QUIET_OFF)) {
    classLock = false;
    motionScore = 0;
    edgeCount = 0;
    if (DEBUG_MODE) {
      Serial.println(F("*** CLASS LOCK RELEASED *** (12 min silence)"));
    }
  }

  lastPirLevel = pirLevel;

  // Keep old name for compatibility
  bool motionDetected = pirLevel;
  
  // Debug output (only if enabled)
  if (DEBUG_MODE) {
    static unsigned long lastDebugPrint = 0;
    static bool lastDebugPirState = LOW;
    
    // Print on state changes or every 5 seconds
    if (motionDetected != lastDebugPirState || (currentTime - lastDebugPrint >= 5000)) {
      unsigned long timeSinceMotion = currentTime - lastMotionTime;
      unsigned long offDelay = pickOffDelay();
      
      Serial.print(F("Time: "));
      Serial.print(currentTime / 1000);
      Serial.print(F("s | PIR: "));
      Serial.print(motionDetected ? F("HIGH") : F("LOW "));
      Serial.print(F(" | State: "));
      
      switch(currentState) {
        case STARTUP: Serial.print(F("STARTUP    ")); break;
        case IDLE: Serial.print(F("IDLE       ")); break;
        case ANIMATING: Serial.print(F("ANIMATING  ")); break;
        case LIGHTS_ON: Serial.print(F("LIGHTS_ON  ")); break;
        case SHUTTING_DOWN: Serial.print(F("SHUTDOWN   ")); break;
      }
      
      Serial.print(F(" | Score: "));
      Serial.print(motionScore);
      Serial.print(F(" | Class: "));
      Serial.print(classLock ? F("LOCKED") : F("no    "));
      Serial.print(F(" | LastMotion: "));
      Serial.print(timeSinceMotion / 1000);
      Serial.print(F("s"));
      
      if (currentState == LIGHTS_ON) {
        Serial.print(F(" | Timeout: "));
        long timeUntilTimeout = (long)(offDelay - timeSinceMotion) / 1000;
        if (timeUntilTimeout > 0) {
          Serial.print(timeUntilTimeout);
          Serial.print(F("s ("));
          if (classLock) Serial.print(F("12min"));
          else if (offDelay == OFF_2MIN) Serial.print(F("2min"));
          else if (offDelay == OFF_30S) Serial.print(F("30s"));
          else Serial.print(F("15s"));
          Serial.print(F(")"));
        } else {
          Serial.print(F("NOW!"));
        }
      }
      
      Serial.println();
      lastDebugPrint = currentTime;
      lastDebugPirState = motionDetected;
    }
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
      // Start on a real edge (less false-triggering), but allow pirLevel in case boot happens mid-HIGH
      if (motionEdge || motionDetected) {
        // Choose animation using counter + random for guaranteed variety
        // Even if random() returns the same value, counter ensures cycling
        animationCounter = (animationCounter + random(1, 4)) % 16;
        currentStory = (StoryType)animationCounter;
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
      unsigned long offDelay = pickOffDelay();
      bool motionTimeout = (currentTime - lastMotionTime >= offDelay);
      
      // IMPORTANT: Tie failsafe to classLock - 2 hours for classes, 15 min otherwise
      unsigned long maxOn = classLock ? CLASS_FAILSAFE_ON : MAX_ON_TIME;
      bool failsafeTimeout = (currentTime - lightsOnTime >= maxOn);
      
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

// ==================== OCCUPANCY HELPERS ====================
void pushEdge(unsigned long t) {
  edgeTimes[edgeHead] = t;
  edgeHead = (edgeHead + 1) % MAX_EDGES;
  if (edgeCount < MAX_EDGES) edgeCount++;
}

int countEdgesInWindow(unsigned long now, unsigned long windowMs) {
  int c = 0;
  for (int i = 0; i < edgeCount; i++) {
    int idx = (edgeHead - 1 - i + MAX_EDGES) % MAX_EDGES;
    unsigned long t = edgeTimes[idx];
    if (now - t <= windowMs) c++;
    else break;
  }
  return c;
}

unsigned long pickOffDelay() {
  if (classLock) return CLASS_LOCK_QUIET_OFF;

  // Map confidence -> your 3 choices
  if (motionScore >= 60) return OFF_2MIN;
  if (motionScore >= 25) return OFF_30S;
  return OFF_SHORT;
}

// ==================== ANIMATION ENGINE ====================
// Returns true when animation is complete
bool runAnimation(unsigned long currentTime) {
  unsigned long elapsed = currentTime - animStepStartTime;
  
  switch (currentStory) {
    // Original 8
    case RISE:       return animRise(elapsed);
    case WHIRLPOOL:  return animWhirlpool(elapsed);
    case HEARTBEAT:  return animHeartbeat(elapsed);
    case PINBALL:    return animPinball(elapsed);
    case PINCER:     return animPincer(elapsed);
    case RADAR:      return animRadar(elapsed);
    case SPARK:      return animSpark(elapsed);
    case COMPASS:    return animCompass(elapsed);
    // New 8
    case LIGHTNING:  return animLightning(elapsed);
    case TIDE:       return animTide(elapsed);
    case ECLIPSE:    return animEclipse(elapsed);
    case FIREFLY:    return animFirefly(elapsed);
    case AVALANCHE:  return animAvalanche(elapsed);
    case PENDULUM:   return animPendulum(elapsed);
    case SERPENT:    return animSerpent(elapsed);
    case NOVA:       return animNova(elapsed);
  }
  return true;  // Should never reach
}

// ==================== ANIMATION: RISE ====================
// Rising Sun - 2.4 seconds
bool animRise(unsigned long elapsed) {
  switch (animStep) {
    case 0:  // 0-500ms: Bottom light (horizon)
      digitalWrite(pDn, HIGH);
      if (elapsed >= 500) { animStep++; animStepStartTime = millis(); }
      break;
    case 1:  // 500-1000ms: Lower diagonals
      digitalWrite(pLL, HIGH); 
      digitalWrite(pLR, HIGH);
      if (elapsed >= 500) { animStep++; animStepStartTime = millis(); }
      break;
    case 2:  // 1000-1500ms: Side lights
      digitalWrite(pLe, HIGH); 
      digitalWrite(pRi, HIGH);
      if (elapsed >= 500) { animStep++; animStepStartTime = millis(); }
      break;
    case 3:  // 1500-2000ms: Upper diagonal
      digitalWrite(pUL, HIGH); 
      digitalWrite(pUR, HIGH);
      if (elapsed >= 500) { animStep++; animStepStartTime = millis(); }
      break;
    case 4:  // 2000-2400ms: Top light (zenith)
      digitalWrite(pUp, HIGH);
      if (elapsed >= 400) return true;
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

// ==================== ANIMATION: LIGHTNING ====================
// Jagged strike from above - 2.5 seconds
// Narrative: "The sky cracks open with fury, electricity arcing downward in violent bursts"
bool animLightning(unsigned long elapsed) {
  switch (animStep) {
    case 0:  // Top flash
      digitalWrite(pUp, HIGH);
      if (elapsed >= 100) { animStep++; animStepStartTime = millis(); }
      break;
    case 1:  // Off
      digitalWrite(pUp, LOW);
      if (elapsed >= 150) { animStep++; animStepStartTime = millis(); }
      break;
    case 2:  // Upper diagonals strike
      digitalWrite(pUL, HIGH); 
      digitalWrite(pUR, HIGH);
      if (elapsed >= 100) { animStep++; animStepStartTime = millis(); }
      break;
    case 3:  // Off
      setAll(LOW);
      if (elapsed >= 150) { animStep++; animStepStartTime = millis(); }
      break;
    case 4:  // Fork left and right
      digitalWrite(pLe, HIGH); 
      digitalWrite(pRi, HIGH);
      if (elapsed >= 150) { animStep++; animStepStartTime = millis(); }
      break;
    case 5:  // Spread to lower diagonals
      digitalWrite(pLL, HIGH); 
      digitalWrite(pLR, HIGH);
      if (elapsed >= 100) { animStep++; animStepStartTime = millis(); }
      break;
    case 6:  // Ground strike
      digitalWrite(pDn, HIGH);
      if (elapsed >= 200) { animStep++; animStepStartTime = millis(); }
      break;
    case 7:  // All flash bright
      setAll(HIGH);
      if (elapsed >= 100) { animStep++; animStepStartTime = millis(); }
      break;
    case 8:  // Flicker off-on
      setAll(LOW);
      if (elapsed >= 100) { animStep++; animStepStartTime = millis(); }
      break;
    case 9:  // Final on
      setAll(HIGH);
      if (elapsed >= 100) return true;
      break;
  }
  return false;
}

// ==================== ANIMATION: TIDE ====================
// Ocean waves rolling in and out - 4 seconds
// Narrative: "The eternal breath of the sea, advancing and retreating across the shore"
bool animTide(unsigned long elapsed) {
  switch (animStep) {
    case 0:  // Wave approaches from bottom
      digitalWrite(pDn, HIGH);
      if (elapsed >= 400) { animStep++; animStepStartTime = millis(); }
      break;
    case 1:  // Wave rises
      digitalWrite(pLL, HIGH); 
      digitalWrite(pLR, HIGH);
      if (elapsed >= 300) { animStep++; animStepStartTime = millis(); }
      break;
    case 2:  // Wave crests
      digitalWrite(pLe, HIGH); 
      digitalWrite(pRi, HIGH);
      if (elapsed >= 300) { animStep++; animStepStartTime = millis(); }
      break;
    case 3:  // Wave retreats from sides
      digitalWrite(pLe, LOW); 
      digitalWrite(pRi, LOW);
      if (elapsed >= 300) { animStep++; animStepStartTime = millis(); }
      break;
    case 4:  // Continues retreating
      digitalWrite(pLL, LOW); 
      digitalWrite(pLR, LOW);
      if (elapsed >= 300) { animStep++; animStepStartTime = millis(); }
      break;
    case 5:  // Back to sea
      digitalWrite(pDn, LOW);
      if (elapsed >= 400) { animStep++; animStepStartTime = millis(); }
      break;
    case 6:  // Second wave comes in stronger - all at once
      digitalWrite(pDn, HIGH);
      digitalWrite(pLL, HIGH); 
      digitalWrite(pLR, HIGH);
      digitalWrite(pLe, HIGH); 
      digitalWrite(pRi, HIGH);
      if (elapsed >= 500) { animStep++; animStepStartTime = millis(); }
      break;
    case 7:  // High tide - everything on
      setAll(HIGH);
      if (elapsed >= 100) return true;
      break;
  }
  return false;
}

// ==================== ANIMATION: ECLIPSE ====================
// Shadow passes across the light - 3 seconds
// Narrative: "Darkness creeps across the face of brightness, then light is reborn"
bool animEclipse(unsigned long elapsed) {
  switch (animStep) {
    case 0:  // Full light
      setAll(HIGH);
      if (elapsed >= 400) { animStep++; animStepStartTime = millis(); }
      break;
    case 1:  // Shadow begins from left
      digitalWrite(pLe, LOW);
      digitalWrite(pUL, LOW);
      digitalWrite(pLL, LOW);
      if (elapsed >= 300) { animStep++; animStepStartTime = millis(); }
      break;
    case 2:  // Totality - only right edge visible
      digitalWrite(pUp, LOW);
      digitalWrite(pDn, LOW);
      if (elapsed >= 500) { animStep++; animStepStartTime = millis(); }
      break;
    case 3:  // Shadow moves right
      digitalWrite(pRi, LOW);
      digitalWrite(pUR, LOW);
      digitalWrite(pLR, LOW);
      if (elapsed >= 300) { animStep++; animStepStartTime = millis(); }
      break;
    case 4:  // All dark at maximum eclipse
      setAll(LOW);
      if (elapsed >= 400) { animStep++; animStepStartTime = millis(); }
      break;
    case 5:  // Light returns from left
      digitalWrite(pLe, HIGH);
      digitalWrite(pUL, HIGH);
      digitalWrite(pLL, HIGH);
      if (elapsed >= 200) { animStep++; animStepStartTime = millis(); }
      break;
    case 6:  // Center returns
      digitalWrite(pUp, HIGH);
      digitalWrite(pDn, HIGH);
      if (elapsed >= 200) { animStep++; animStepStartTime = millis(); }
      break;
    case 7:  // Full light restored
      setAll(HIGH);
      if (elapsed >= 100) return true;
      break;
  }
  return false;
}

// ==================== ANIMATION: FIREFLY ====================
// Random twinkling dance - 4 seconds
// Narrative: "Scattered embers of living light wink on and off in the summer darkness"
bool animFirefly(unsigned long elapsed) {
  static int glowPattern[8];
  static bool fireflyInitialized = false;
  
  // Initialize random glow pattern
  if (animStep == 0 && !fireflyInitialized) {
    for (int i = 0; i < NUM_LEDS; i++) {
      glowPattern[i] = random(100, 400);
    }
    fireflyInitialized = true;
  }
  
  if (animStep < 20) {  // 20 random flickers
    // Pick random LED
    int led = random(0, NUM_LEDS);
    digitalWrite(ledPins[led], HIGH);
    
    if (elapsed >= random(100, 300)) {
      digitalWrite(ledPins[led], LOW);
      animStep++;
      animStepStartTime = millis();
    }
  } else if (animStep == 20) {
    // All fireflies glow together briefly
    setAll(HIGH);
    if (elapsed >= 200) {
      animStep++;
      animStepStartTime = millis();
    }
  } else if (animStep == 21) {
    setAll(LOW);
    if (elapsed >= 150) {
      animStep++;
      animStepStartTime = millis();
    }
  } else {
    // Final all-on
    setAll(HIGH);
    fireflyInitialized = false;
    return true;
  }
  return false;
}

// ==================== ANIMATION: AVALANCHE ====================
// Cascading from top to bottom - 2 seconds
// Narrative: "The mountain breaks, snow tumbling downward in unstoppable momentum"
bool animAvalanche(unsigned long elapsed) {
  switch (animStep) {
    case 0:  // Peak breaks
      digitalWrite(pUp, HIGH);
      if (elapsed >= 200) { animStep++; animStepStartTime = millis(); }
      break;
    case 1:  // Upper slopes tumble
      digitalWrite(pUL, HIGH); 
      digitalWrite(pUR, HIGH);
      if (elapsed >= 250) { animStep++; animStepStartTime = millis(); }
      break;
    case 2:  // Sides caught in flow
      digitalWrite(pLe, HIGH); 
      digitalWrite(pRi, HIGH);
      if (elapsed >= 250) { animStep++; animStepStartTime = millis(); }
      break;
    case 3:  // Lower slopes overwhelmed
      digitalWrite(pLL, HIGH); 
      digitalWrite(pLR, HIGH);
      if (elapsed >= 250) { animStep++; animStepStartTime = millis(); }
      break;
    case 4:  // Crashes to valley floor
      digitalWrite(pDn, HIGH);
      if (elapsed >= 300) { animStep++; animStepStartTime = millis(); }
      break;
    case 5:  // Dust settles - all buried
      setAll(HIGH);
      if (elapsed >= 100) return true;
      break;
  }
  return false;
}

// ==================== ANIMATION: PENDULUM ====================
// Swinging side to side with increasing speed - 3.5 seconds
// Narrative: "Time's relentless swing, back and forth, faster and faster until all moments blur"
bool animPendulum(unsigned long elapsed) {
  const int swings[] = {pLe, pRi, pLe, pRi, pLe, pRi, pLe, pRi};
  const int delays[] = {500, 500, 400, 400, 300, 300, 200, 200};
  
  if (animStep < 8) {
    // Turn off previous
    setAll(LOW);
    // Light current position
    digitalWrite(swings[animStep], HIGH);
    // Add opposite diagonal for arc effect
    if (swings[animStep] == pLe) {
      if (animStep < 6) digitalWrite(pUL, HIGH);
      else { digitalWrite(pUL, HIGH); digitalWrite(pLL, HIGH); }
    } else {
      if (animStep < 6) digitalWrite(pUR, HIGH);
      else { digitalWrite(pUR, HIGH); digitalWrite(pLR, HIGH); }
    }
    
    if (elapsed >= delays[animStep]) {
      animStep++;
      animStepStartTime = millis();
    }
  } else {
    // Pendulum stops at center - all on
    setAll(HIGH);
    return true;
  }
  return false;
}

// ==================== ANIMATION: SERPENT ====================
// Slithering clockwise then counter-clockwise - 3.5 seconds
// Narrative: "An ancient serpent coils around the compass, weaving its eternal pattern"
bool animSerpent(unsigned long elapsed) {
  const int clockwise[] = {pUp, pUR, pRi, pLR, pDn, pLL, pLe, pUL};
  
  if (animStep < 8) {
    // Clockwise slither - turn on next, keep last 2 on
    digitalWrite(clockwise[animStep], HIGH);
    if (animStep >= 3) {
      digitalWrite(clockwise[animStep - 3], LOW);
    }
    if (elapsed >= 200) {
      animStep++;
      animStepStartTime = millis();
    }
  } else if (animStep < 16) {
    // Counter-clockwise - reverse direction
    int idx = 15 - animStep;
    digitalWrite(clockwise[idx], HIGH);
    // Turn off tail
    if (animStep >= 11) {
      digitalWrite(clockwise[idx + 3], LOW);
    }
    if (elapsed >= 200) {
      animStep++;
      animStepStartTime = millis();
    }
  } else {
    // Serpent coils - all on
    setAll(HIGH);
    return true;
  }
  return false;
}

// ==================== ANIMATION: NOVA ====================
// Stellar explosion from center - 2.5 seconds
// Narrative: "A dying star's final cry - energy bursting outward in all directions at once"
bool animNova(unsigned long elapsed) {
  switch (animStep) {
    case 0:  // Core ignition - all quick flash
      setAll(HIGH);
      if (elapsed >= 50) { animStep++; animStepStartTime = millis(); }
      break;
    case 1:  // Collapse
      setAll(LOW);
      if (elapsed >= 200) { animStep++; animStepStartTime = millis(); }
      break;
    case 2:  // Core bright again
      setAll(HIGH);
      if (elapsed >= 50) { animStep++; animStepStartTime = millis(); }
      break;
    case 3:  // Collapse deeper
      setAll(LOW);
      if (elapsed >= 300) { animStep++; animStepStartTime = millis(); }
      break;
    case 4:  // Cardinal directions explode first
      digitalWrite(pUp, HIGH); 
      digitalWrite(pDn, HIGH);
      digitalWrite(pLe, HIGH); 
      digitalWrite(pRi, HIGH);
      if (elapsed >= 150) { animStep++; animStepStartTime = millis(); }
      break;
    case 5:  // Diagonals explode
      digitalWrite(pUL, HIGH); 
      digitalWrite(pUR, HIGH);
      digitalWrite(pLL, HIGH); 
      digitalWrite(pLR, HIGH);
      if (elapsed >= 150) { animStep++; animStepStartTime = millis(); }
      break;
    case 6:  // Full supernova brightness
      setAll(HIGH);
      if (elapsed >= 100) { animStep++; animStepStartTime = millis(); }
      break;
    case 7:  // Flicker
      setAll(LOW);
      if (elapsed >= 50) { animStep++; animStepStartTime = millis(); }
      break;
    case 8:  // Final blaze
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
