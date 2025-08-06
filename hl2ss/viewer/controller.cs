using UnityEngine;
using MixedReality.Toolkit.UX;
using TMPro;


public class SineWaveController : MonoBehaviour
{
    public Slider sliderFrequency;
    public Slider sliderAmplitude;
    public Slider sliderAngle;
    public Slider sliderLength;
    public Slider sliderResolution;

    public TMP_Text freqLabel, ampLabel, samplesLabel, angleLabel, lengthLabel;

    public SineRope rope;

    void Start()
    {
        sliderFrequency.OnValueUpdated.AddListener(data => UpdateRope());
        sliderAmplitude.OnValueUpdated.AddListener(data => UpdateRope());
        sliderAngle.OnValueUpdated.AddListener(data => UpdateRope());
        sliderLength.OnValueUpdated.AddListener(data => UpdateRope());
        sliderResolution.OnValueUpdated.AddListener(data => UpdateRope());

        UpdateRope(); // Initial update
    }

    public void UpdateRope()
    {
        float freq = sliderFrequency.Value;
        float amp = sliderAmplitude.Value;
        float angle = sliderAngle.Value;
        float length = sliderLength.Value;
        float res = sliderResolution.Value;


        freqLabel.text = $"Freq: {freq:F2}";
        ampLabel.text = $"Amp: {amp:F2}";
        samplesLabel.text = $"Res: {res:F1}";
        angleLabel.text = $"Angle: {angle:F1}°";
        lengthLabel.text = $"Length: {length:F2}m";

        rope.UpdateWaveParams(freq, amp, res, angle, length);
    }
}