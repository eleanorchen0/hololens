using Unity.VisualScripting;
using UnityEngine;
using UnityEngine.UIElements;

[RequireComponent(typeof(MeshFilter), typeof(MeshRenderer))]
public class PatchAntenna : MonoBehaviour
{
    [Header("resolution")]
    public int phiSegments = 64;
    public int thetaSegments = 32;

    [Header("patch dimensions")]
    public float patchWidth = 1.0f;
    public float patchLength = 1.0f;

    [Header("scale")]
    public float radiusScale = 1.0f;

    private Mesh mesh;

    void Awake()
    {
        mesh = new Mesh { name = "PatchAntenna" };
        GetComponent<MeshFilter>().mesh = mesh;
        BuildMesh();
    }

    void OnValidate()
    {
        if (mesh != null)
            BuildMesh();
    }

    void BuildMesh()
    {
        mesh.Clear();

        int vertCount = (phiSegments + 1) * (thetaSegments + 1);
        Vector3[] vertices = new Vector3[vertCount];
        Vector2[] uvs = new Vector2[vertCount];
        int[] triangles = new int[phiSegments * thetaSegments * 6];

        float k = 2f * Mathf.PI;
        float halfKW = k * patchWidth * 0.5f;
        float halfKL = k * patchLength * 0.5f;

        int vi = 0;
        for (int ti = 0; ti <= thetaSegments; ti++)
        {
            float theta = Mathf.PI * ti / thetaSegments;        // 0 → π
            float sinT = Mathf.Sin(theta);
            float cosT = Mathf.Cos(theta);

            for (int pi = 0; pi <= phiSegments; pi++)
            {
                float phi = 2f * Mathf.PI * pi / phiSegments;   // 0 → 2π
                float cosP = Mathf.Cos(phi);
                float sinP = Mathf.Sin(phi);

                float xW = halfKW * sinT * cosP;
                float xL = halfKL * sinT * sinP;
                float sW = (Mathf.Approximately(xW, 0f) ? 1f : Mathf.Sin(xW) / xW);
                float sL = (Mathf.Approximately(xL, 0f) ? 1f : Mathf.Sin(xL) / xL);

                //*****************PATTERN****************************
                float pattern = Mathf.Max(sW * sL * cosT, 0f);

                float r = pattern * radiusScale;
                float x = r * sinT * cosP;
                float y = r * sinT * sinP;
                float z = r * cosT;

                vertices[vi] = new Vector3(x, y, z);
                uvs[vi] = new Vector2(pi / (float)phiSegments, pattern);
                vi++;
            }
        }

        int tii = 0;
        for (int ti = 0; ti < thetaSegments; ti++)
        {
            for (int pi = 0; pi < phiSegments; pi++)
            {
                int a = ti * (phiSegments + 1) + pi;
                int b = (ti + 1) * (phiSegments + 1) + pi;
                int c = (ti + 1) * (phiSegments + 1) + pi + 1;
                int d = ti * (phiSegments + 1) + pi + 1;

                triangles[tii++] = a;
                triangles[tii++] = b;
                triangles[tii++] = c;

                triangles[tii++] = a;
                triangles[tii++] = c;
                triangles[tii++] = d;
            }
        }

        mesh.vertices = vertices;
        mesh.uv = uvs;
        mesh.triangles = triangles;
        mesh.RecalculateNormals();
    }

    public void UpdateParams(float width, float length, float scale)
    {
        patchWidth = 1/width;
        patchLength = 1/length;
        radiusScale = scale;

        BuildMesh();
    }
}