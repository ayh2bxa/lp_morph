class AGC
{
public:
    AGC(float fs)
      : sampleRate(fs),
        envelope(0.0),
        gainSmoothed(1.0f)
    {
        // set parameter defaults:
        targetLevel_dB = -20.0f;
        attackTime_ms  = 5.0f;
        releaseTime_ms = 30.0f;
        rmsTime_ms     = 10.0f;  // for RMS detector

        // compute one-pole coefficients:
        attackCoeff  = expf(-1.0f / (0.001f * attackTime_ms  * sampleRate));
        releaseCoeff = expf(-1.0f / (0.001f * releaseTime_ms * sampleRate));
        rmsCoeff     = expf(-1.0f / (0.001f * rmsTime_ms     * sampleRate));
    }

    // Call this once per sample:
    float processSample(float inSample)
    {
        // 1) Envelope detection (RMS)
        double x2 = sqrt(inSample * inSample);
//        envelope = sqrt(rmsCoeff * (envelope * envelope) + (1.0f - rmsCoeff) * x2);
//        if (envelope == 0.0) {
//            envelope += 1e-20;
//        }
        if (x2 == 0.0) {
            x2 += 1e-20;
        }
        // 2) Convert to dB
        float level_dB    = 20.0 * log10(x2);

        // 3) Compute desired gain (dB) and clamp
        float desired_dB    = targetLevel_dB - level_dB;
        if (desired_dB > 0.f) {
            desired_dB = 0.f;
        }
        // 4) Convert to linear
        float desiredGain = powf(10.0f, 0.05f * desired_dB);

        // 5) Attack/release smoothing
        if (desiredGain < gainSmoothed)
            gainSmoothed = attackCoeff  * (gainSmoothed - desiredGain) + desiredGain;
        else
            gainSmoothed = releaseCoeff * (gainSmoothed - desiredGain) + desiredGain;

        // 6) Apply gain
        return inSample * desiredGain;
    }

    // Expose setters if you want to change parameters at runtime:
    void setTargetLevel_dB(float dB)   { targetLevel_dB = dB; }
    void setAttackTime_ms(float t_ms)  { attackTime_ms = t_ms; attackCoeff  = expf(-1.0f/(0.001f*t_ms*sampleRate)); }
    void setReleaseTime_ms(float t_ms) { releaseTime_ms = t_ms; releaseCoeff = expf(-1.0f/(0.001f*t_ms*sampleRate)); }

private:
    // State
    double envelope;
    float gainSmoothed;

    // Parameters
    float sampleRate;
    float targetLevel_dB;
    float maxGain_dB;
    float minGain_dB;
    float attackTime_ms;
    float releaseTime_ms;
    float rmsTime_ms;

    // One-pole coefficients
    float attackCoeff;
    float releaseCoeff;
    float rmsCoeff;
    size_t counter = 0;
};
