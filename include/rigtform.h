#ifndef RIGTFORM_H
#define RIGTFORM_H

#include <cassert>
#include <iostream>

#include "matrix4.h"
#include "quat.h"

class RigTForm {
  Cvec3 t_;// translation component
  Quat r_; // rotation component represented as a quaternion

  public:
  RigTForm() : t_(0) {
    assert(norm2(Quat(1, 0, 0, 0) - r_) < CS175_EPS2);
  }

  RigTForm(const Cvec3 &t, const Quat &r) : t_(t), r_(r) {}

  explicit RigTForm(const Cvec3 &t) : t_(t), r_() {}

  explicit RigTForm(const Quat &r) : t_(), r_(r) {}

  Cvec3 getTranslation() const {
    return t_;
  }

  Quat getRotation() const {
    return r_;
  }

  RigTForm &setTranslation(const Cvec3 &t) {
    t_ = t;
    return *this;
  }

  RigTForm &setRotation(const Quat &r) {
    r_ = r;
    return *this;
  }

  Cvec4 operator*(const Cvec4 &a) const {
    return r_ * a + Cvec4(t_, 0) * a[3];
  }

  RigTForm operator*(const RigTForm &a) const {
    return RigTForm(
            t_ + Cvec3(r_ * Cvec4(a.getTranslation(), 0)),
            r_ * a.getRotation());
  }

  friend std::ostream &operator<<(std::ostream &out, const RigTForm &rigTForm) {
    out << rigTForm.t_[0] << "," << rigTForm.t_[1] << "," << rigTForm.t_[2] << ","
        << rigTForm.r_[0] << "," << rigTForm.r_[1] << "," << rigTForm.r_[2] << "," << rigTForm.r_[3] << ";";

    return out;
  };
};

inline RigTForm inv(const RigTForm &tform) {
  Quat r_inv = inv(tform.getRotation());
  return RigTForm(
          Cvec3(r_inv * Cvec4(-tform.getTranslation(), 1)),
          r_inv);
}

inline RigTForm transFact(const RigTForm &tform) {
  return RigTForm(tform.getTranslation());
}

inline RigTForm linFact(const RigTForm &tform) {
  return RigTForm(tform.getRotation());
}

inline RigTForm matrixToRigTForm(const Matrix4 &mat) {
  return RigTForm(Cvec3(mat(0, 3), mat(1, 3), mat(2, 3)), matrixToQuat(mat));
}

inline Matrix4 rigTFormToMatrix(const RigTForm &tform) {
  Matrix4 T = Matrix4::makeTranslation(tform.getTranslation());
  Matrix4 R = quatToMatrix(tform.getRotation());
  return T * R;
}

inline RigTForm interpolate(const RigTForm &rbt0, const RigTForm &rbt1, double alpha) {
  return RigTForm(lerp(rbt0.getTranslation(), rbt1.getTranslation(), alpha),
                  slerp(rbt0.getRotation(), rbt1.getRotation(), alpha));
}

inline RigTForm controlPoint(const RigTForm &rbt_minus, const RigTForm &rbt, const RigTForm &rbt_plus, double sign) {
  static const double one_sixth = (double)1 / (double)6;

  Cvec3 vec_minus = rbt_minus.getTranslation();
  Cvec3 vec = rbt.getTranslation();
  Cvec3 vec_plus = rbt_plus.getTranslation();

  Cvec3 trans = (vec_plus - vec_minus) * one_sixth * sign + vec;

  Quat quat_minus = rbt_minus.getRotation();
  Quat quat = rbt.getRotation();
  Quat quat_plus = rbt.getRotation();

  Quat rot = (quat_plus == quat_minus) ? quat : pow(cn(quat_plus * inv(quat_minus)), one_sixth * sign) * quat;

  return RigTForm(trans, rot);
}

inline RigTForm catmullRomInterpolate(const RigTForm &rbt0, const RigTForm &rbt1, const RigTForm &rbt2, const RigTForm &rbt3, double alpha) {

  RigTForm ctrlPt0 = controlPoint(rbt0, rbt1, rbt2, 1.0);
  RigTForm ctrlPt1 = controlPoint(rbt1, rbt2, rbt3, -1.0);

  RigTForm t0 = interpolate(rbt1, ctrlPt0, alpha);
  RigTForm t1 = interpolate(ctrlPt0, ctrlPt1, alpha);
  RigTForm t2 = interpolate(ctrlPt1, rbt2, alpha);
  RigTForm t3 = interpolate(t0, t1, alpha);
  RigTForm t4 = interpolate(t1, t2, alpha);

  return interpolate(t3, t4, alpha);
}

#endif
