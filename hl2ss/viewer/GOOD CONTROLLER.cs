using UnityEngine;
using MixedReality.Toolkit.UX;
using TMPro;


public class SineWaveController : MonoBehaviour
{
    public Slider sliderFrequency;
    public Slider sliderAmplitude;
    public Slider sliderResolution;

    public TMP_Text freqLabel, ampLabel, samplesLabel;

    public SineRope rope;
    public ReflectedRope reflection;
    public TransmittedRope transmission;

    void Start()
    {
        sliderFrequency.OnValueUpdated.AddListener(data => UpdateRope());
        sliderAmplitude.OnValueUpdated.AddListener(data => UpdateRope());
        sliderResolution.OnValueUpdated.AddListener(data => UpdateRope());

        UpdateRope();
    }

    public void UpdateRope()
    {
        float freq = sliderFrequency.Value;
        float amp = sliderAmplitude.Value;
        float res = sliderResolution.Value;


        freqLabel.text = $"Freq: {freq:F2}";
        ampLabel.text = $"Amp: {amp:F2}";
        samplesLabel.text = $"Res: {res:F1}";

        rope.UpdateWaveParams(freq, amp, res);
        reflection.UpdateWaveParams(freq, amp - 0.01f, res);
        transmission.UpdateWaveParams(freq, amp - 0.075f, res);
    }
}