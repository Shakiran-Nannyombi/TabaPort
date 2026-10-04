"""TabaPort GIS: Analytical Hierarchy Process weights and consistency ratio.
Author: TabaPort GIS and Geomatics workstream.
Runs offline with numpy only."""
import numpy as np

RI = {1: 0.0, 2: 0.0, 3: 0.58, 4: 0.90, 5: 1.12, 6: 1.24, 7: 1.32, 8: 1.41, 9: 1.45}

def ahp(names, upper):
    """upper: dict {(i, j): value} for i < j on the Saaty 1 to 9 scale."""
    n = len(names)
    A = np.ones((n, n))
    for (i, j), v in upper.items():
        A[i, j] = v
        A[j, i] = 1.0 / v
    vals, vecs = np.linalg.eig(A)
    k = np.argmax(vals.real)
    w = np.abs(vecs[:, k].real)
    w = w / w.sum()
    lam = vals[k].real
    ci = (lam - n) / (n - 1)
    cr = ci / RI[n] if RI[n] else 0.0
    return A, w, lam, ci, cr

# 1. Static flood susceptibility (computed in Google Earth Engine)
S_NAMES = ["HAND", "Low-point index", "Flow accumulation (TWI)",
           "Distance to drainage", "Imperviousness", "Slope", "Rainfall extremes"]
S_UPPER = {
    (0, 1): 2, (0, 2): 2, (0, 3): 3, (0, 4): 3, (0, 5): 5, (0, 6): 7,
    (1, 2): 1, (1, 3): 2, (1, 4): 2, (1, 5): 4, (1, 6): 6,
    (2, 3): 2, (2, 4): 2, (2, 5): 3, (2, 6): 5,
    (3, 4): 1, (3, 5): 2, (3, 6): 4,
    (4, 5): 2, (4, 6): 4,
    (5, 6): 2,
}

# 2. Dispatch Tactical Priority Score (the team's three IEEE criteria)
D_NAMES = ["Velocity of water rise", "Proximity to critical infrastructure",
           "Neighbourhood population density"]
D_UPPER = {(0, 1): 2, (0, 2): 3, (1, 2): 2}

if __name__ == "__main__":
    for title, names, up in [("STATIC FLOOD SUSCEPTIBILITY", S_NAMES, S_UPPER),
                             ("DISPATCH TACTICAL PRIORITY", D_NAMES, D_UPPER)]:
        A, w, lam, ci, cr = ahp(names, up)
        print(title)
        for nm, wi in zip(names, w):
            print(f"  {nm:40s} {wi:.3f}")
        print(f"  lambda_max={lam:.3f}  CI={ci:.4f}  CR={cr:.4f}  "
              f"{'CONSISTENT (CR < 0.10)' if cr < 0.1 else 'REVISE'}")
        print()
