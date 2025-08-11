using UnityEngine;
using System.Collections.Generic;

[RequireComponent(typeof(MeshFilter), typeof(MeshRenderer))]
public class ReflectedRope : MonoBehaviour
{
    [Header("Link to sine")]
    [SerializeField] private SineRope rope;
    [SerializeField] private client client;

    [Header("Wave Settings")]
    public float ropeLength = 2f;
    public float sineAmplitude = 0.5f;
    public float sineFrequency = 1f;

    [Header("Mesh Resolution")]
    public float samplesPerMeter = 10f;
    public int radialSegments = 8;
    public float ropeRadius = 0.1f;

    [Header("Centerline Rope")]
    public bool showCenterline = true;
    public Material centerlineMaterial;
    public float centerlineRopeRadius = 0.03f;

    [Header("Cloth Settings")]
    public bool showCloth = true;
    public Material clothMaterial;

    [Header("Directional Control")]
    public Vector3 startPoint = Vector3.zero;
    [Tooltip("Angle in degrees from +X axis, in the XY plane.")]
    public float angle = 0f;

    // internally computed
    private Vector3 waveDirection;
    private Vector3 sineDirection;

    private int pathResolution;

    // Called in the Editor whenever a field changes
    private void OnValidate()
    {
        ropeLength = Mathf.Max(0f, ropeLength);
        samplesPerMeter = Mathf.Max(0.1f, samplesPerMeter);

        ComputeDirections();
        pathResolution = Mathf.Max(2, Mathf.CeilToInt(ropeLength * samplesPerMeter));
    }

    void Start()
    {
        ComputeDirections();
        pathResolution = Mathf.Max(2, Mathf.CeilToInt(ropeLength * samplesPerMeter));

        GenerateSineRope();
        if (showCenterline) GenerateCenterlineRope();
        if (showCloth) GenerateClothBetweenCenterAndSine();
    }

    private void Update()
    {
        // pull the latest length from your linked SineRope
        UpdateLength(rope.ropeLength);
        angle = client.angle;
      
    }

    // Recompute wave and sine directions from the single 'angle' property
    private void ComputeDirections()
    {
        float rad = angle * Mathf.Deg2Rad;
        waveDirection = new Vector3(Mathf.Cos(rad), Mathf.Sin(rad), 0f).normalized;
        sineDirection = Vector3.Cross(waveDirection, Vector3.forward).normalized;
    }

    void GenerateSineRope()
    {
        Mesh mesh = GenerateRopeMesh(
            pointFunc: i =>
            {
                float t = i / (float)(pathResolution - 1);
                float dist = ropeLength * t;
                float offset = Mathf.Sin(dist * sineFrequency) * sineAmplitude;
                Vector3 basePt = startPoint + waveDirection * dist;
                return basePt + sineDirection * offset;
            },
            radius: ropeRadius
        );

        GetComponent<MeshFilter>().mesh = mesh;
    }

    void GenerateCenterlineRope()
    {
        GameObject centerline = new GameObject("CenterlineRope");
        centerline.transform.SetParent(transform, false);

        var mf = centerline.AddComponent<MeshFilter>();
        var mr = centerline.AddComponent<MeshRenderer>();
        if (centerlineMaterial != null)
            mr.material = centerlineMaterial;

        Mesh mesh = GenerateRopeMesh(
            pointFunc: i =>
            {
                float t = i / (float)(pathResolution - 1);
                float dist = ropeLength * t;
                return startPoint + waveDirection * dist;
            },
            radius: centerlineRopeRadius
        );
        mf.mesh = mesh;
    }

    void GenerateClothBetweenCenterAndSine()
    {
        GameObject cloth = new GameObject("ClothMesh");
        cloth.transform.SetParent(transform, false);

        var mf = cloth.AddComponent<MeshFilter>();
        var mr = cloth.AddComponent<MeshRenderer>();
        if (clothMaterial != null)
            mr.material = clothMaterial;

        var verts = new List<Vector3>();
        var tris = new List<int>();
        var uvs = new List<Vector2>();

        for (int i = 0; i < pathResolution; i++)
        {
            float t = i / (float)(pathResolution - 1);
            float dist = ropeLength * t;
            float off = Mathf.Sin(dist * sineFrequency) * sineAmplitude;

            Vector3 c = startPoint + waveDirection * dist;
            Vector3 s = c + sineDirection * off;

            verts.Add(c);
            verts.Add(s);
            uvs.Add(new Vector2(0, t));
            uvs.Add(new Vector2(1, t));
        }

        for (int i = 0; i < pathResolution - 1; i++)
        {
            int a = i * 2, b = a + 1, c = a + 2, d = a + 3;
            // front
            tris.Add(a); tris.Add(d); tris.Add(b);
            tris.Add(a); tris.Add(c); tris.Add(d);
            // back
            tris.Add(b); tris.Add(d); tris.Add(a);
            tris.Add(d); tris.Add(c); tris.Add(a);
        }

        Mesh mesh = new Mesh { name = "ClothMesh" };
        mesh.SetVertices(verts);
        mesh.SetUVs(0, uvs);
        mesh.SetTriangles(tris, 0);
        mesh.RecalculateNormals();
        mf.mesh = mesh;
    }

    Mesh GenerateRopeMesh(System.Func<int, Vector3> pointFunc, float radius)
    {
        var verts = new List<Vector3>();
        var tris = new List<int>();
        var uvs = new List<Vector2>();

        Vector3[] path = new Vector3[pathResolution];
        Vector3[] tangents = new Vector3[pathResolution];

        for (int i = 0; i < pathResolution; i++)
        {
            path[i] = pointFunc(i);
            tangents[i] = (i > 0)
                ? (path[i] - path[i - 1]).normalized
                : waveDirection;
        }

        for (int i = 0; i < pathResolution; i++)
        {
            Vector3 tangent = tangents[i];
            Vector3 normal = Vector3.up;
            Vector3 binorm = Vector3.Cross(tangent, normal).normalized;
            normal = Vector3.Cross(binorm, tangent).normalized;

            for (int j = 0; j < radialSegments; j++)
            {
                float a = 2 * Mathf.PI * j / radialSegments;
                Vector3 offset = Mathf.Cos(a) * normal * radius
                               + Mathf.Sin(a) * binorm * radius;
                verts.Add(path[i] + offset);
                uvs.Add(new Vector2(j / (float)radialSegments, i / (float)pathResolution));
            }
        }

        for (int i = 0; i < pathResolution - 1; i++)
        {
            for (int j = 0; j < radialSegments; j++)
            {
                int curr = i * radialSegments + j;
                int next = (i + 1) * radialSegments + j;
                int currNext = i * radialSegments + (j + 1) % radialSegments;
                int nextNext = (i + 1) * radialSegments + (j + 1) % radialSegments;

                tris.Add(curr); tris.Add(nextNext); tris.Add(next);
                tris.Add(curr); tris.Add(currNext); tris.Add(nextNext);
            }
        }

        Mesh mesh = new Mesh { name = "GeneratedRopeMesh" };
        mesh.SetVertices(verts);
        mesh.SetUVs(0, uvs);
        mesh.SetTriangles(tris, 0);
        mesh.RecalculateNormals();
        return mesh;
    }

    public void UpdateWaveParams(float frequency, float amplitude, float samples)
    {
        sineFrequency = frequency;
        sineAmplitude = amplitude;
        samplesPerMeter = samples;

        foreach (Transform child in transform)
            Destroy(child.gameObject);
        Start();
    }

    public void UpdateLength(float length)
    {
        ropeLength = length;
        foreach (Transform child in transform)
            Destroy(child.gameObject);
        Start();
    }
}