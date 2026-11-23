# macOS signing and notarization

Both products ship their Qt6 app for the Mac, signed with the same Apple
Developer ID (team `7Q3QN9AV9J`): the **Developer ID Application**
certificate and the hardened runtime, notarized and stapled, in a `.dmg`.
Both are signed in CI, by the `mac-aarch64` job of
[`.github/workflows/qt-desktop.yml`](../.github/workflows/qt-desktop.yml),
which the product's workflow runs as its `desktop` job, with the
repository secrets below:

| Product | Mac app | Workflow | Bundle identifier |
|---|---|---|---|
| Calc-U-59 | `products/calc-u-59/frontends/qt6/` | [`.github/workflows/calc-u-59.yml`](../.github/workflows/calc-u-59.yml) | `ch.erzberger.CalcU59Qt` |
| Calc-U-1600 | `products/calc-u-1600/frontends/qt6/` | [`.github/workflows/calc-u-1600.yml`](../.github/workflows/calc-u-1600.yml) | `ch.erzberger.CalcU1600Qt` |

The bundle identifiers are `ch.erzberger.<CMake target>`, set by the library's `calcu_qt6_app` (`library/qt6/CMakeLists.txt`).
Calc-U-59's iOS app (`products/calc-u-59/frontends/swift/`) goes to the App
Store through Xcode's own signing (`release ios`), not with these
certificates.

## The CI job

Each `mac-aarch64` job builds the `.app`, then:

1. Installs it (`cmake --install build/release --prefix
   build/release/install`, the library's `calcu_qt6_app`) and runs
   `macdeployqt` on the installed `.app` to embed the Qt frameworks, so
   the bundle doesn't depend on Homebrew's Qt on the machine that runs it.
2. Runs the repository's action
   [`.github/actions/mac-sign-dmg`](../.github/actions/mac-sign-dmg/action.yml),
   which does the rest. It signs the `.app` with the Developer ID
   Application certificate and the hardened runtime.
3. Notarizes and staples the `.app`.
4. Packages it into a `.dmg` (with an `/Applications` symlink for
   drag-install), then signs, notarizes and staples the `.dmg` too.

A `.dmg` is signed with the Developer ID Application certificate directly;
no *Developer ID Installer* certificate or `.pkg` is needed.

## Repository secrets

| Secret | Contents |
|---|---|
| `APPLE_CERT_APPLICATION_P12_BASE64` | base64 of the `Developer ID Application` certificate's `.p12` |
| `APPLE_CERT_INSTALLER_P12_BASE64`   | base64 of the `Developer ID Installer` certificate's `.p12` (set by `renew-certs apply`; no workflow uses it) |
| `APPLE_CERT_P12_PASSWORD`           | the `.p12` export password (the same for both) |
| `APPLE_API_KEY_P8_BASE64`           | base64 of the App Store Connect API key (`AuthKey_XXXXXXXXXX.p8`) |
| `APPLE_API_KEY_ID`                  | that key's ID |
| `APPLE_API_ISSUER_ID`               | the issuer UUID |

The certificates are exported from the Mac's keychain that holds them
(both private keys); `renew-certs apply` (below) does that. The App Store
Connect API key (`.p8`) is a **one-time Apple download**: if the file is
lost, generate a new key rather than trying to recover it. Set by hand:

```sh
gh secret set APPLE_CERT_APPLICATION_P12_BASE64 < <(base64 -i DeveloperIDApplication.p12)
gh secret set APPLE_CERT_INSTALLER_P12_BASE64   < <(base64 -i DeveloperIDInstaller.p12)
gh secret set APPLE_API_KEY_P8_BASE64           < <(base64 -i AuthKey_XXXXXXXXXX.p8)
printf %s 'THE_EXPORT_PASSWORD'                  | gh secret set APPLE_CERT_P12_PASSWORD
printf %s 'ABCDE12345'                           | gh secret set APPLE_API_KEY_ID
printf %s '69a6de7f-1a2b-3c4d-5e6f-70a1b2c3d4e5' | gh secret set APPLE_API_ISSUER_ID
```

## Rotating

Developer ID certificates from the G2 Sub-CA are valid only ~13 months.
Each year: create new ones in the developer portal with the **G2 Sub-CA**
(not Xcode, which issued from the original Sub-CA that expires
2027-02-01). The steps are listed by `renew-certs guide`
([ReleaseIt](https://github.com/tinue/ReleaseIt)). Then run
`renew-certs apply` in `products/calc-u-1600/`, where its `release.yaml`
is (the team ID comes from its `certs:` section). It exports the certificates from
the keychain, checks them, and sets the two `_P12_BASE64` secrets and
`APPLE_CERT_P12_PASSWORD`. The same certificates sign SharpDataExchange;
run `apply` there too. It also records the uploaded certificates as the
repository variables `APPLE_CERT_{APPLICATION,INSTALLER}_{SHA1,EXPIRES}`;
`renew-certs status` reports them and their expiry. The `.p12` password is
random per run and kept nowhere else; if a secret breaks, run `apply`
again. Back the certificates up with `renew-certs backup` (1Password,
Private vault; restore by double-clicking the attached `.p12`).

The App Store Connect API key doesn't expire. For a new one, replace the
`.p8` secret plus `APPLE_API_KEY_ID` / `APPLE_API_ISSUER_ID`.
