import React from "react";
import classnames from "classnames";
import Layout from "@theme/Layout";
import Link from "@docusaurus/Link";
import useDocusaurusContext from "@docusaurus/useDocusaurusContext";
import useBaseUrl from "@docusaurus/useBaseUrl";
import styles from "./styles.module.css";

function Home() {
  const context = useDocusaurusContext();
  const { siteConfig = {} } = context;
  return (
    <Layout
      title={`${siteConfig.title} - ${siteConfig.tagline}`}
      description="Description will go into a meta tag in <head />"
    >
      <header className={classnames("hero", styles.heroBanner)}>
        <div className="container">
          <div className="row">
            <div className="col">
              <h1 className="hero__title" style={{ color: "#29C7AC" }}>
                {siteConfig.title}
              </h1>
              <p className="hero__subtitle">{siteConfig.tagline}</p>
              <div className={styles.installCommandWrapper}>
                <code
                  className={classnames("highlight", styles.installCommand)}
                >
                  npm i -g pesy
                </code>{" "}
              </div>
              <div className={styles.buttons}>
                <Link
                  className={classnames(
                    "button button--outline button--secondary button--lg",
                    styles.getStarted
                  )}
                  to={useBaseUrl("docs/introduction")}
                >
                  Get Started
                </Link>
              </div>
            </div>
            <div className="col">
              <img src="img/demo.gif" alt="demo" />
            </div>
          </div>
        </div>
      </header>
      <main className={classnames("container")}>
        <h2 className={styles.ph2}>
          Convenience features your esy project was looking for
        </h2>
        <section className="row">
          <section className={classnames("col", styles.featuresList)}>
            <ul>
              <li> NPM like require statements to import packages. </li>
              <li> Use package.json itself to configure the build </li>
              <li> Pre warmed esy cache so that builds finish faster </li>
              <li> Out of the box useful templates </li>
            </ul>
          </section>
          <section className="col">
            <img src="./img/sample.png" alt="Sample project" />
          </section>
        </section>
      </main>
    </Layout>
  );
}

export default Home;
