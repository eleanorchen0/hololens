using UnityEngine;
using System.Collections.Generic;
using UnityEngine.UIElements;

[RequireComponent(typeof(MeshFilter), typeof(MeshRenderer))]
public class HornAntenna : MonoBehaviour
{
    [Header("controller")]
    public controller controller;

    [Header("resolution")]
    public int phiSegments = 120;
    public int thetaSegments = 60;

    [Header("DIMENSIONS")]
    public float apertureWidth = 2.0f;
    public float apertureHeight = 1.0f;

    [Header("scale")]
    public float radiusScale = 1.0f;
    public float zOffset = 0.0f;

    [Header("de bruyne")]
    public float minGain = -20f;
    public float maxGain = 18f;

    [Header("MATERIAL")]
    public Material surface;
    public Material gridMaterial;

    [Header("boost for the short ones")]
    [Range(0.1f, 2f)]
    public float gamma = 0.5f;

    private Mesh mesh;
    private float div;

    private bool doubled;


    void Awake()
    {
        mesh = new Mesh { name = "HornPattern" };
        GetComponent<MeshFilter>().mesh = mesh;
        GetComponent<MeshRenderer>().materials = new Material[] { surface, gridMaterial };

        BuildMesh();
    }

    void OnValidate()
    {
        if (mesh != null)
            BuildMesh();
        

        GetComponent<MeshRenderer>().materials = new Material[] { surface, gridMaterial };

    }

    void BuildMesh()
    {
        doubled = controller.doubled;
        if (doubled)
        {
            div = 1f;
        }
        else
        {
            div = 2f;
        }

        mesh.Clear();

        int vCount = (phiSegments + 1) * (thetaSegments + 1);
        var verts = new Vector3[vCount];
        var uvs = new Vector2[vCount];

        var tris = new List<int>();
        var lines = new List<int>();

        float k = 2f * Mathf.PI;
        float halfKW = k * apertureWidth * 0.5f;
        float halfKH = k * apertureHeight * 0.5f;

        int vi = 0;
        // ************CALCS*************
        for (int ti = 0; ti <= thetaSegments; ti++)
        {

            float theta = Mathf.PI * ti / div / thetaSegments;
            float sinT = Mathf.Sin(theta);
            float cosT = Mathf.Cos(theta);

            for (int pi = 0; pi <= phiSegments; pi++)
            {
                float phi = Mathf.PI * 0.5f + Mathf.PI * pi / phiSegments;
                float cosP = Mathf.Cos(phi);
                float sinP = Mathf.Sin(phi);

                float xArg = halfKW * sinT * cosP;
                float yArg = halfKH * sinT * sinP;
                float sincX = Mathf.Approximately(xArg, 0f) ? 1f : Mathf.Sin(xArg) / xArg;
                float sincY = Mathf.Approximately(yArg, 0f) ? 1f : Mathf.Sin(yArg) / yArg;
                float pattern = Mathf.Abs(sincX * sincY);

                float gainDB = 20f * Mathf.Log10(pattern + 1e-6f);
                float t = Mathf.InverseLerp(minGain, maxGain, gainDB);
                t = Mathf.Clamp01(t);
                t = Mathf.Pow(t, gamma);

                Vector3 dir = new Vector3(sinT * cosP, sinT * sinP, cosT);
                verts[vi] = dir * (radiusScale * (t + 0.01f)) + new Vector3(0, 0, zOffset);
                uvs[vi] = new Vector2(t, 0);

                vi++;
            }
        }

        for (int ti = 0; ti < thetaSegments; ti++)
        {
            for (int pi = 0; pi < phiSegments; pi++)
            {
                int a = ti * (phiSegments + 1) + pi;
                int b = (ti + 1) * (phiSegments + 1) + pi;
                int c = b + 1;
                int d = a + 1;

                tris.Add(a); tris.Add(b); tris.Add(c);
                tris.Add(a); tris.Add(c); tris.Add(d);

                lines.Add(a); lines.Add(b);
                lines.Add(a); lines.Add(d);
            }
        }

        for (int ti = 0; ti < thetaSegments; ti++)
        {
            int a = ti * (phiSegments + 1) + phiSegments;
            int b = (ti + 1) * (phiSegments + 1) + phiSegments;
            lines.Add(a); lines.Add(b);
        }

        for (int pi = 0; pi < phiSegments; pi++)
        {
            int a = thetaSegments * (phiSegments + 1) + pi;
            int d = a + 1;
            lines.Add(a); lines.Add(d);
        }

        mesh.subMeshCount = 1;
        mesh.vertices = verts;
        mesh.uv = uvs;
        mesh.SetIndices(tris.ToArray(), MeshTopology.Triangles, 0);

        mesh.RecalculateNormals();
        mesh.RecalculateBounds();

        mesh.subMeshCount = 2;
        mesh.SetIndices(lines.ToArray(), MeshTopology.Lines, 1);
    }

    public void UpdateParams(float width, float height, float scale, float dbmin, float dbmax, float gamma1)
    {
        apertureHeight = height;
        apertureWidth = width;
        radiusScale = scale;
        minGain = dbmin;
        maxGain = dbmax;
        gamma = gamma1;
        doubled = controller.doubled;

        BuildMesh();
    }

    
}