
#define PIN_CLOCK_1   4
#define PIN_CLOCK_2   5
#define PIN_CLOCK_3   6
#define PIN_CLOCK_4   7

void update_cv_outs(unsigned long ticks) {
    // start bar (every fourth quarter note)
    if (ticks%(PPQN*4)==0) {
      /*Midi1.sendContinue();
      Midi2.sendContinue();
      Midi3.sendContinue();*/
     
      digitalWrite(4, HIGH);
    } else if (ticks%(PPQN*4)==duration) {
      digitalWrite(4, LOW);
    }

    // every two quarter notes
    if (ticks%(PPQN*2)==0) {
      digitalWrite(5, HIGH);
    } else if (ticks%(PPQN*2)==duration) {
      digitalWrite(5, LOW);
    }

    // every quarter note
    if (ticks%(PPQN)==0) {
      digitalWrite(6, HIGH);

    } else if (ticks%PPQN==duration) {
      digitalWrite(6, LOW);
    }

    // every sixteenth note
    if (ticks%(PPQN/2)==0) {
      digitalWrite(7, HIGH);
      //digitalWrite(6, LOW);
      //digitalWrite(7, LOW);
    } else if (ticks%(PPQN/2)==duration) {
      digitalWrite(7, LOW);
    }

}
