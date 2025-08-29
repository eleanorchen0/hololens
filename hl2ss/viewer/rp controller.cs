using UnityEngine;
using MixedReality.Toolkit.UX;
using TMPro;


public class SineWaveController : MonoBehaviour
{
    public Slider dipoleRadius;

    public Slider patchWidth;
    public Slider patchLength;
    public Slider patchScale;

    public Slider hornWidth;
    public Slider hornHeight;
    public Slider radiusScale;
    public Slider dbMin;
    public Slider dbMax;
    public Slider gamma;

    public TMP_Text dr;
    public TMP_Text pw, pl, ps;
    public TMP_Text hw, hh, rs, dbmin, dbmax, g;

    public DipoleAntenna dipole;
    public PatchAntenna patch;
    public HornAntenna hornLeft;
    public HornAntenna hornRight;

    void Start()
    {
        UpdatePatterns();

        dipoleRadius.OnValueUpdated.AddListener(data => UpdatePatterns());

        patchWidth.OnValueUpdated.AddListener(data => UpdatePatterns());
        patchLength.OnValueUpdated.AddListener(data => UpdatePatterns());
        patchScale.OnValueUpdated.AddListener(data => UpdatePatterns());

        hornWidth.OnValueUpdated.AddListener(data => UpdatePatterns());
        hornHeight.OnValueUpdated.AddListener(data => UpdatePatterns());
        radiusScale.OnValueUpdated.AddListener(data => UpdatePatterns());
        dbMin.OnValueUpdated.AddListener(data => UpdatePatterns());
        dbMax.OnValueUpdated.AddListener(data => UpdatePatterns());
        gamma.OnValueUpdated.AddListener(data => UpdatePatterns());

    }

    public void UpdatePatterns()
    {
        float r1 = dipoleRadius.Value;

        float w1 = patchWidth.Value;
        float l1 = patchLength.Value;
        float s1 = patchScale.Value;

        float w2 = hornWidth.Value;
        float l2 = hornHeight.Value;
        float r2 = radiusScale.Value;
        float dbm1 = dbMin.Value;
        float dbm2 = dbMax.Value;
        float gam = gamma.Value;


        dr.text = $"Dipole Scale: {r1:F2}";

        pw.text = $"Patch Width: {w1:F2}";
        pl.text = $"Patch Length: {l1:F2}";
        ps.text = $"Patch Scale: {s1:F2}";

        hw.text = $"Horn Width: {w2:F2}";
        hh.text = $"Horn Height: {l2:F2}";
        rs.text = $"Radius Scale: {r2:F2}";
        dbmin.text = $"DB Min: {dbm1:F2}";
        dbmax.text = $"DB Max: {dbm2:F2}";
        g.text = $"Gamma: {gam:F2}";

        dipole.UpdateParams(r1);
        patch.UpdateParams(w1, l1, s1);
        hornLeft.UpdateParams(w2, l2, r2, dbm1, dbm2, gam);
        hornRight.UpdateParams(w2, l2, r2, dbm1, dbm2, gam);
    }

}