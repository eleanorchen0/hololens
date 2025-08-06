using UnityEngine;
using System.Collections.Generic;

[RequireComponent(typeof(MeshFilter), typeof(MeshRenderer))]
public class SineRope : MonoBehaviour
{
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
    public Vector3 waveDirection = Vector3.right;   // Direction rope flows
    public Vector3 sineDirection = Vector3.up;      // Direction sine wave displaces

    private int pathResolution;

    void Start()
    {
        pathResolution = Mathf.Max(2, Mathf.CeilToInt(ropeLength * samplesPerMeter));

        GenerateSineRope();

        if (showCenterline)
        {
            GenerateCenterlineRope();
        }

        if (showCloth)
        {
            GenerateClothBetweenCenterAndSine();
        }
    }

    void OnValidate()
    {
        if (waveDirection == Vector3.zero) waveDirection = Vector3.right;
        if (sineDirection == Vector3.zero) sineDirection = Vector3.up;
        waveDirection.Normalize();
        sineDirection.Normalize();
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

    void GenerateCenterlineRope()
    {
        GameObject centerline = new GameObject("CenterlineRope");
        centerline.transform.parent = this.transform;
        centerline.transform.localPosition = Vector3.zero;

        MeshFilter mf = centerline.AddComponent<MeshFilter>();
        MeshRenderer mr = centerline.AddComponent<MeshRenderer>();
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

            // Front face
            triangles.Add(i0); triangles.Add(i3); triangles.Add(i1);
            triangles.Add(i0); triangles.Add(i2); triangles.Add(i3);

            // Back face (reversed)
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
            if (i > 0)
                tangents[i] = (path[i] - path[i - 1]).normalized;
            else
                tangents[i] = waveDirection; // default initial direction
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

                triangles.Add(current);
                triangles.Add(nextNext);
                triangles.Add(next);

                triangles.Add(current);
                triangles.Add(currentNext);
                triangles.Add(nextNext);
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

    public void UpdateWaveParams(float frequency, float amplitude, float samples, float planeAngleDegrees, float length)
    {
        sineFrequency = frequency;
        sineAmplitude = amplitude;
        samplesPerMeter = samples;
        ropeLength = length;

        // Convert angle (degrees) to direction in XY plane
        float radians = planeAngleDegrees * Mathf.Deg2Rad;
        sineDirection = new Vector3(Mathf.Cos(radians), Mathf.Sin(radians), 0f);

        // Regenerate everything
        foreach (Transform child in transform)
            Destroy(child.gameObject);  // destroy cloth and centerline
        Start();  // reinitialize rope
    }

}
