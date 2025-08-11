using UnityEngine;
using System.Collections.Generic;
[RequireComponent(typeof(MeshFilter), typeof(MeshRenderer))]
public class SineRope : MonoBehaviour
{
    [Header("Wave Settings")] public float sineAmplitude = 0.5f; public float sineFrequency = 1f;
    [Header("Mesh Resolution")]
    public float samplesPerMeter = 10f;
    public int radialSegments = 8;
    public float ropeRadius = 0.1f;

    [Header("Cloth Settings")]
    public bool showCloth = true;
    public Material clothMaterial;

    [Header("Endpoints")]
    public Vector3 startPoint = new Vector3(1, 1, 4);
    public Vector3 endPoint = new Vector3(5, -6, 4);
    public Vector3 sineDirection = Vector3.up;

    private int pathResolution;
    public Vector3 waveDirection;
    public float ropeLength;
    public float rotation;
    public GameObject material;

    void Start()
    {
        if (!Application.isPlaying)
            return;

        UpdateDirectionAndLength();
        pathResolution = Mathf.Max(2, Mathf.CeilToInt(ropeLength * samplesPerMeter));

        GenerateSineRope();

        if (showCloth)
            GenerateClothBetweenCenterAndSine();
    }

    void OnValidate()
    {
        UpdateDirectionAndLength();
        sineDirection = Vector3.up;
        sineDirection.Normalize();
    }

    void Awake()
    {
        GetComponent<MeshFilter>().mesh = null; // Clear any serialized mesh
    }

    void UpdateDirectionAndLength()
    {
        waveDirection = (endPoint - startPoint).normalized;
        ropeLength = Vector3.Distance(startPoint, endPoint);
        sineDirection = Vector3.Cross(waveDirection, Vector3.forward).normalized;
        material.transform.position = endPoint;
        material.transform.rotation = Quaternion.Euler(0, CalculateAngle(startPoint, endPoint), 0);
    }

    void GenerateSineRope()
    {

        Mesh mesh = GenerateRopeMesh(
            pointFunc: i =>
            {
                float t = i / (float)(pathResolution - 1);
                float dist = ropeLength * t;
                float sineOffset = Mathf.Sin(dist * sineFrequency) * sineAmplitude;
                Vector3 basePoint = startPoint + waveDirection * dist;
                return basePoint + sineDirection * sineOffset;
            },
            radius: ropeRadius
        );

        GetComponent<MeshFilter>().mesh = mesh;
    }
    void GenerateClothBetweenCenterAndSine()
    {

        GameObject cloth = new GameObject("ClothMesh");
        cloth.transform.parent = this.transform;
        cloth.transform.localPosition = Vector3.zero;

        MeshFilter mf = cloth.AddComponent<MeshFilter>();
        MeshRenderer mr = cloth.AddComponent<MeshRenderer>();
        if (clothMaterial != null)
            mr.material = clothMaterial;

        List<Vector3> vertices = new List<Vector3>();
        List<int> triangles = new List<int>();
        List<Vector2> uvs = new List<Vector2>();

        for (int i = 0; i < pathResolution; i++)
        {
            float t = i / (float)(pathResolution - 1);
            float dist = ropeLength * t;
            float sineOffset = Mathf.Sin(dist * sineFrequency) * sineAmplitude;

            Vector3 centerPoint = startPoint + waveDirection * dist;
            Vector3 sinePoint = centerPoint + sineDirection * sineOffset;

            vertices.Add(centerPoint);
            vertices.Add(sinePoint);

            uvs.Add(new Vector2(0, t));
            uvs.Add(new Vector2(1, t));
        }

        for (int i = 0; i < pathResolution - 1; i++)
        {
            int i0 = i * 2;
            int i1 = i0 + 1;
            int i2 = i0 + 2;
            int i3 = i0 + 3;

            triangles.Add(i0); triangles.Add(i3); triangles.Add(i1);
            triangles.Add(i0); triangles.Add(i2); triangles.Add(i3);

            triangles.Add(i1); triangles.Add(i3); triangles.Add(i0);
            triangles.Add(i3); triangles.Add(i2); triangles.Add(i0);
        }

        Mesh mesh = new Mesh();
        mesh.name = "ClothMesh";
        mesh.SetVertices(vertices);
        mesh.SetTriangles(triangles, 0);
        mesh.SetUVs(0, uvs);
        mesh.RecalculateNormals();

        mf.mesh = mesh;
    }

    Mesh GenerateRopeMesh(System.Func<int, Vector3> pointFunc, float radius)
    {

        List<Vector3> vertices = new List<Vector3>();
        List<int> triangles = new List<int>();
        List<Vector2> uvs = new List<Vector2>();

        Vector3[] path = new Vector3[pathResolution];
        Vector3[] tangents = new Vector3[pathResolution];

        for (int i = 0; i < pathResolution; i++)
        {
            path[i] = pointFunc(i);
            tangents[i] = i > 0 ? (path[i] - path[i - 1]).normalized : waveDirection;
        }

        for (int i = 0; i < pathResolution; i++)
        {
            Vector3 tangent = tangents[i];
            Vector3 normal = Vector3.up;
            Vector3 binormal = Vector3.Cross(tangent, normal).normalized;
            normal = Vector3.Cross(binormal, tangent).normalized;

            for (int j = 0; j < radialSegments; j++)
            {
                float angle = 2 * Mathf.PI * j / radialSegments;
                Vector3 offset = Mathf.Cos(angle) * normal * radius + Mathf.Sin(angle) * binormal * radius;
                vertices.Add(path[i] + offset);
                uvs.Add(new Vector2((float)j / radialSegments, (float)i / pathResolution));
            }
        }

        for (int i = 0; i < pathResolution - 1; i++)
        {
            for (int j = 0; j < radialSegments; j++)
            {
                int current = i * radialSegments + j;
                int next = (i + 1) * radialSegments + j;
                int currentNext = i * radialSegments + (j + 1) % radialSegments;
                int nextNext = (i + 1) * radialSegments + (j + 1) % radialSegments;

                triangles.Add(current); triangles.Add(nextNext); triangles.Add(next);
                triangles.Add(current); triangles.Add(currentNext); triangles.Add(nextNext);
            }
        }

        Mesh mesh = new Mesh();
        mesh.name = "GeneratedRopeMesh";
        mesh.SetVertices(vertices);
        mesh.SetTriangles(triangles, 0);
        mesh.SetUVs(0, uvs);
        mesh.RecalculateNormals();
        return mesh;
    }

    private float CalculateAngle(Vector3 start, Vector3 end)
    {
        return Vector3.Angle(start, end);
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

    public void UpdateStart(Vector3 start)
    {
        startPoint = start;

        foreach (Transform child in transform)
            Destroy(child.gameObject);
        Start();
    }

    public void UpdateEnd(Vector3 end)
    {
        endPoint = end;

        foreach (Transform child in transform)
            Destroy(child.gameObject);
        Start();
    }

}
